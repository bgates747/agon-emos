from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import re
import tempfile
import unittest
import zlib


ROOT = Path(__file__).resolve().parents[1]
MOS_SOURCE = ROOT
MODULE_PATH = ROOT / "projects" / "emos" / "emos_module.py"
SPEC = importlib.util.spec_from_file_location("emos_module", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
emos = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(emos)


def manifest(provider_class: str = "service", namespace: str = "edu", name: str = "probe") -> dict:
    return {
        "schema": 1,
        "class": provider_class,
        "namespace": namespace,
        "name": name,
        "version": [1, 0, 0],
        "core_abi": [1, 1],
        "entry_offset": 128,
        "request_size": [24, 24],
        "flags": 0,
        "capabilities": 0,
    }


class EmosModuleTests(unittest.TestCase):
    def test_four_mode_shapes_and_legacy_hub_transactions(self) -> None:
        self.assertEqual(
            emos.MODE_SHAPES,
            {0: (0, 0), 1: (0, 1), 2: (1, 1), 3: (2, 1)},
        )
        legacy = {"mode": 0, "vdu_route": 0, "edu_active": 0, "generation": 4}
        unavailable = emos.mode_transaction(legacy, 1, "unavailable")
        self.assertEqual(unavailable["status"], emos.STATUS_UNAVAILABLE)
        self.assertEqual(unavailable["state"], legacy)
        self.assertEqual(unavailable["events"], ["prepare", "recover"])

        dual = emos.mode_transaction(legacy, 1, "fake")
        self.assertEqual(dual["status"], 0)
        self.assertEqual(
            dual["events"], ["prepare", "ready", "adapter-commit", "commit"]
        )
        self.assertEqual(
            dual["state"],
            {"mode": 1, "vdu_route": 0, "edu_active": 1, "generation": 5},
        )
        direct = emos.mode_transaction(dual["state"], 2, "fake")
        self.assertEqual(direct["status"], emos.STATUS_UNAVAILABLE)
        self.assertEqual(direct["state"], dual["state"])
        back = emos.mode_transaction(dual["state"], 0, "fake")
        self.assertEqual(back["events"], ["recover", "commit"])
        self.assertEqual(back["state"]["mode"], 0)

        for exclusive in (2, 3):
            with self.subTest(exclusive=exclusive):
                denied = emos.mode_transaction(legacy, exclusive, "fake")
                self.assertEqual(denied["status"], emos.STATUS_UNAVAILABLE)
                self.assertEqual(denied["state"], legacy)

    def test_target_mode_state_is_separate_and_committed_last(self) -> None:
        source = (MOS_SOURCE / "src" / "emos.c").read_text(
            encoding="utf-8"
        )
        required = [
            "typedef struct {\n\tBYTE mode;\n\tBYTE vduRoute;\n\tBYTE eduState;",
            "static t_emosEduState emosEduState;",
            "result = emos_adapter_prepare(mode);",
            "result = emos_adapter_ready(mode);",
            "result = emos_adapter_commit(mode);",
            "emos_adapter_recover();",
            "emosModeState = prepared;",
            "emosVduBackend = prepared.vduRoute;",
        ]
        for expression in required:
            with self.subTest(expression=expression):
                self.assertIn(expression, source)
        self.assertNotIn("mos_sysvars", source[source.index("typedef struct {\n\tBYTE mode;"):])
    @staticmethod
    def executable_header(
        *, version: int = 1, mode: int = 1, flags: int = 1, address: int = 0
    ) -> bytes:
        image = bytearray(0x4A)
        image[0x40:0x43] = b"MOS"
        image[0x43] = version
        image[0x44] = mode
        image[0x45] = flags
        image[0x46] = (~flags) & 0xFF
        image[0x47:0x4A] = address.to_bytes(3, "little")
        return bytes(image)

    def test_application_eligibility_matrix(self) -> None:
        load = 0x040000
        cases = [
            (b"short", load, emos.POLICY_UNSAFE),
            (self.executable_header(version=0), load, emos.POLICY_UNSAFE),
            (self.executable_header(mode=0), load, emos.POLICY_UNSAFE),
            (self.executable_header(flags=0), load, emos.POLICY_UNSAFE),
            (self.executable_header(flags=1), load, emos.POLICY_SAFE),
            (self.executable_header(flags=2), load, emos.POLICY_COMPATIBLE),
            (self.executable_header(flags=3), load, emos.POLICY_UNSAFE),
            (self.executable_header(flags=4), load, emos.POLICY_UNSAFE),
            (
                self.executable_header(flags=1, address=0x050000),
                load,
                emos.POLICY_UNSAFE,
            ),
            (
                self.executable_header(flags=1, address=load),
                load,
                emos.POLICY_SAFE,
            ),
            (self.executable_header(flags=1), emos.MODULE_BASE, emos.POLICY_MOSLET),
        ]
        inverse = bytearray(self.executable_header(flags=1))
        inverse[0x46] ^= 1
        cases.append((bytes(inverse), load, emos.POLICY_UNSAFE))
        for image, address, expected in cases:
            with self.subTest(address=address, expected=expected):
                self.assertEqual(emos.application_policy(image, address), expected)

    def test_target_preservation_is_bounded_and_recoverable(self) -> None:
        source = (MOS_SOURCE / "src" / "emos.c").read_text(
            encoding="utf-8"
        )
        required = [
            "if (restore && f_size(&file) != EMOS_MODULE_SIZE)",
            "if (!restore && result == FR_OK) result = f_sync(&file);",
            "if (!restore && result != FR_OK) f_unlink(EMOS_SWAP_PATH);",
            "emosRecoveryRequired = TRUE;",
            "The interrupted request is never replayed implicitly.",
            "if (emosRecoveryRequired) {",
            "emosRecoveryRequired && previousPolicy == EMOS_POLICY_CORE",
            "emos_read24(image + 0x47) != address",
        ]
        for expression in required:
            with self.subTest(expression=expression):
                self.assertIn(expression, source)

        mos_source = (MOS_SOURCE / "src" / "mos.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("BYTE previousPolicy = emos_application_enter", mos_source)
        self.assertIn("result = exec16(addr, args);", mos_source)
        self.assertIn("result = exec24(addr, args);", mos_source)
        self.assertIn("emos_application_leave(previousPolicy);", mos_source)

    def test_compatible_preservation_failure_order(self) -> None:
        save_failure = emos.preservation_transaction(save_status=1)
        self.assertEqual(save_failure["events"], ["save", "discard-partial"])
        self.assertFalse(save_failure["provider_called"])
        self.assertFalse(save_failure["recovery_required"])

        restore_failure = emos.preservation_transaction(
            provider_status=0, restore_status=1
        )
        self.assertEqual(
            restore_failure["status"], emos.STATUS_RECOVERY_FAILED
        )
        self.assertTrue(restore_failure["provider_called"])
        self.assertTrue(restore_failure["recovery_required"])

        provider_failure = emos.preservation_transaction(provider_status=33)
        self.assertEqual(provider_failure["status"], 33)
        self.assertEqual(provider_failure["events"][-1], "discard-swap")
    def test_discovery_transaction_success_is_sorted_and_atomic(self) -> None:
        previous = {
            "generation": 7,
            "entries": [{"name": "retained"}],
        }
        alpha = emos.build_container(manifest(name="alpha"), b"a")
        beta = emos.build_container(manifest(name="beta"), b"b")
        published = emos.discover_registry(
            previous, [("z.emo", beta), ("a.emo", alpha)]
        )
        self.assertEqual(published["generation"], 8)
        self.assertEqual(
            [entry["name"] for entry in published["entries"]], ["alpha", "beta"]
        )
        self.assertEqual(previous["entries"], [{"name": "retained"}])
        emptied = emos.discover_registry(published, [])
        self.assertEqual(emptied, {"generation": 9, "entries": []})

    def test_every_discovery_failure_retains_previous_registry(self) -> None:
        previous = {
            "generation": 11,
            "entries": [{"name": "retained"}],
        }
        good = emos.build_container(manifest(), b"payload")
        corrupt = bytearray(good)
        corrupt[-1] ^= 1
        incompatible = bytearray(good)
        incompatible[14:16] = b"\x02\x02"
        incompatible[emos.HEADER_CRC_OFFSET : emos.HEADER_CRC_OFFSET + 4] = b"\0" * 4
        incompatible[
            emos.HEADER_CRC_OFFSET : emos.HEADER_CRC_OFFSET + 4
        ] = zlib.crc32(incompatible[: emos.HEADER_SIZE]).to_bytes(4, "little")

        def missing():
            raise OSError("module directory does not exist")
            yield  # pragma: no cover

        def interrupted():
            yield "good.emo", good
            raise OSError("directory read interrupted")

        failures = [
            lambda: emos.discover_registry(previous, [("bad.emo", bytes(corrupt))]),
            lambda: emos.discover_registry(
                previous, [("future.emo", bytes(incompatible))]
            ),
            lambda: emos.discover_registry(previous, [("a.emo", good), ("b.emo", good)]),
            lambda: emos.discover_registry(
                previous,
                [(f"{index}.emo", good) for index in range(17)],
            ),
            lambda: emos.discover_registry(
                previous, [("good.emo", good)], allocation_available=False
            ),
            lambda: emos.discover_registry(previous, interrupted()),
            lambda: emos.discover_registry(previous, missing()),
        ]
        for action in failures:
            with self.subTest(action=action), self.assertRaises(
                (emos.ModuleError, OSError)
            ):
                action()
            self.assertEqual(
                previous,
                {"generation": 11, "entries": [{"name": "retained"}]},
            )

    def test_target_discovery_publishes_only_after_complete_validation(self) -> None:
        source = (MOS_SOURCE / "src" / "emos.c").read_text(
            encoding="utf-8"
        )
        self.assertIn("staging = umm_malloc(sizeof(t_emosRegistry));", source)
        self.assertIn("result = emos_validate_file(candidate.path, &candidate);", source)
        self.assertIn("if (result == FR_OK) emosRegistry = *staging;", source)
        self.assertIn("generation = emosRegistry.generation + 1;", source)
        self.assertIn("emosRegistry.generation = generation;", source)
        self.assertLess(
            source.index("if (result == FR_OK) emosRegistry = *staging;"),
            source.index("umm_free(staging);"),
        )

    def test_target_gateway_rejects_non_service_requests(self) -> None:
        source = (MOS_SOURCE / "src" / "emos.c").read_text(
            encoding="utf-8"
        )
        self.assertIn(
            "emos_read16(request->operation) != EMOS_OPERATION_SERVICE", source
        )
        self.assertIn("request->namespaceLength == 0", source)

    def test_machine_readable_negative_fixtures(self) -> None:
        fixture_path = ROOT / "projects" / "emos" / "fixtures" / "cases.json"
        cases = json.loads(fixture_path.read_text(encoding="utf-8"))
        self.assertEqual(
            [case["name"] for case in cases],
            [
                "malformed-identity",
                "incompatible-core",
                "oversize-image",
                "unsupported-capability",
                "duplicate-claim",
            ],
        )
        for case in cases:
            with self.subTest(name=case["name"]):
                document = manifest()
                document.update(case["manifest_overrides"])
                payload = b"x" * case["payload_size"]
                if case.get("copies"):
                    image = emos.build_container(document, payload)
                    action = lambda: emos.validate_registry(
                        [(f"{index}.emo", image) for index in range(case["copies"])]
                    )
                else:
                    action = lambda: emos.build_container(document, payload)
                with self.assertRaisesRegex(emos.ModuleError, case["error"]):
                    action()

    def test_shipped_manifests_are_canonical_bounded_fixtures(self) -> None:
        manifests = ROOT / "projects" / "emos" / "manifests"
        expected = {
            "hello.json": ("star", "", "hello"),
            "echo.json": ("service", "core", "echo"),
            "edu-probe.json": ("service", "edu", "probe"),
        }
        for filename, identity in expected.items():
            with self.subTest(filename=filename):
                document = json.loads((manifests / filename).read_text(encoding="utf-8"))
                normalized = emos.normalize_manifest(document)
                self.assertEqual(
                    (normalized["class"], normalized["namespace"], normalized["name"]),
                    identity,
                )

    def test_target_header_constants_match_host_contract(self) -> None:
        header = (MOS_SOURCE / "src" / "emos.h").read_text(
            encoding="utf-8"
        )
        expected = {
            "EMOS_FORMAT_MAJOR": emos.FORMAT_MAJOR,
            "EMOS_FORMAT_MINOR": emos.FORMAT_MINOR,
            "EMOS_CORE_ABI": emos.CORE_ABI,
            "EMOS_HEADER_SIZE": emos.HEADER_SIZE,
            "EMOS_MODULE_BASE": emos.MODULE_BASE,
            "EMOS_MODULE_SIZE": emos.MODULE_LIMIT,
            "EMOS_PROVIDER_REQUEST_SIZE": emos.PROVIDER_REQUEST_SIZE,
            "EMOS_GATEWAY_REQUEST_SIZE": 66,
            "EMOS_PROVIDER_STAR": emos.CLASS_STAR,
            "EMOS_PROVIDER_SERVICE": emos.CLASS_SERVICE,
            "EMOS_MAX_PROVIDERS": 16,
            "EMOS_NAMESPACE_SIZE": emos.NAMESPACE_SIZE,
            "EMOS_NAME_SIZE": emos.NAME_SIZE,
        }
        definitions = {
            name: int(value, 0)
            for name, value in re.findall(
                r"^#define\s+(EMOS_[A-Z0-9_]+)\s+(0x[0-9A-Fa-f]+|[0-9]+)\s*$",
                header,
                re.MULTILINE,
            )
        }
        self.assertEqual({name: definitions[name] for name in expected}, expected)

    def test_container_is_deterministic_and_round_trips(self) -> None:
        payload = bytes(range(1, 80))
        first = emos.build_container(manifest(), payload)
        second = emos.build_container(dict(reversed(list(manifest().items()))), payload)
        self.assertEqual(first, second)
        info = emos.validate_container(first)
        self.assertEqual(info["class"], "service")
        self.assertEqual(info["namespace"], "edu")
        self.assertEqual(info["name"], "probe")
        self.assertEqual(info["image_size"], 128 + len(payload))

    def test_star_and_service_namespace_rules(self) -> None:
        star = manifest("star", "", "hello")
        self.assertEqual(emos.validate_container(emos.build_container(star, b"x"))["name"], "hello")
        with self.assertRaisesRegex(emos.ModuleError, "namespace"):
            emos.build_container(manifest("service", "", "probe"), b"x")
        with self.assertRaisesRegex(emos.ModuleError, "namespace"):
            emos.build_container(manifest("star", "edu", "probe"), b"x")

    def test_rejects_corruption_reserved_fields_and_oversize(self) -> None:
        image = bytearray(emos.build_container(manifest(), b"payload"))
        image[-1] ^= 1
        with self.assertRaisesRegex(emos.ModuleError, "payload CRC"):
            emos.validate_container(image)
        image = bytearray(emos.build_container(manifest(), b"payload"))
        image[100] = 1
        with self.assertRaisesRegex(emos.ModuleError, "reserved"):
            emos.validate_container(image)
        with self.assertRaisesRegex(emos.ModuleError, "exceeds"):
            emos.build_container(manifest(), b"x" * (emos.MODULE_LIMIT - emos.HEADER_SIZE + 1))

    def test_rejects_bad_manifest_ranges_and_unknown_fields(self) -> None:
        cases = []
        value = manifest(); value["core_abi"] = [2, 1]; cases.append(value)
        value = manifest(); value["request_size"] = [25, 24]; cases.append(value)
        value = manifest(); value["flags"] = 1; cases.append(value)
        value = manifest(); value["name"] = "Upper"; cases.append(value)
        value = manifest(); value["surprise"] = 1; cases.append(value)
        for value in cases:
            with self.subTest(value=value), self.assertRaises(emos.ModuleError):
                emos.build_container(value, b"payload")

    def test_fixed_core_request_must_be_inside_provider_range(self) -> None:
        value = manifest()
        value["request_size"] = [25, 30]
        with self.assertRaisesRegex(emos.ModuleError, "request size range"):
            emos.build_container(value, b"payload")

        value = manifest()
        value["request_size"] = [24, 30]
        self.assertEqual(
            emos.validate_container(emos.build_container(value, b"payload"))[
                "request_size"
            ],
            [24, 30],
        )

    def test_target_dispatch_revalidates_snapshot_and_scrubs(self) -> None:
        source = (MOS_SOURCE / "src" / "emos.c").read_text(
            encoding="utf-8"
        )
        required = [
            "!emos_entry_same_image(entry, &loaded)",
            "if (emosBusy) return EMOS_BUSY;",
            "emos_range_overlaps_module(emos_read24(request->input)",
            "emos_range_overlaps_module(emos_read24(request->output)",
            "memcmp(request, &requestSnapshot, 20)",
            "emos_read24(request->outputLength) > emos_read24(request->outputCapacity)",
            "else emos_scrub_module_area();",
        ]
        for expression in required:
            with self.subTest(expression=expression):
                self.assertIn(expression, source)

    def test_module_area_overlap_boundaries_and_wrap_fail_closed(self) -> None:
        base = emos.MODULE_BASE
        end = base + emos.MODULE_LIMIT
        cases = [
            (base - 1, 1, False),
            (base - 1, 2, True),
            (base, 1, True),
            (end - 1, 1, True),
            (end, 1, False),
            (base, 0, False),
            (0xFFFFF0, 0x20, True),
        ]
        for address, length, expected in cases:
            with self.subTest(address=address, length=length):
                self.assertEqual(
                    emos.range_overlaps_module(address, length), expected
                )

    def test_registry_sort_is_enumeration_independent_and_collisions_fail(self) -> None:
        alpha = emos.build_container(manifest(name="alpha"), b"a")
        beta = emos.build_container(manifest(name="beta"), b"b")
        entries = emos.validate_registry([("z.emo", beta), ("a.emo", alpha)])
        self.assertEqual([entry["name"] for entry in entries], ["alpha", "beta"])
        with self.assertRaisesRegex(emos.ModuleError, "duplicate provider"):
            emos.validate_registry([("one.emo", alpha), ("two.emo", alpha)])

    def test_cli_refuses_output_replacement(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            output = root / "provider.emo"
            output.write_bytes(b"existing")
            with self.assertRaisesRegex(emos.ModuleError, "refusing to replace"):
                emos._write_output(output, b"new")
            self.assertEqual(output.read_bytes(), b"existing")


if __name__ == "__main__":
    unittest.main()
