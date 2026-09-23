import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import sys

sys.dont_write_bytecode = True

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("embed", ROOT / "tools/embed_compatibility.py")
embed = importlib.util.module_from_spec(spec)
spec.loader.exec_module(embed)


class GeneratorTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.game = self.root / "Test Game"
        self.game.mkdir()
        (self.game / "UE4SS-settings.ini").write_bytes(b"\xef\xbb\xbf[Hooks]\r\nHookEngineTick = 0\r\n")
        self.manifest = self.root / "validated-builds.json"
        self.builds([])

    def builds(self, builds):
        self.manifest.write_text(json.dumps({"schemaVersion": 1, "builds": builds}), encoding="utf-8")

    def test_normalizes_text_and_ignores_docs(self):
        (self.game / "README.md").write_text("Not a runtime asset")
        resources = embed.collect(self.root)
        self.assertEqual(resources, [("Test Game", "UE4SS-settings.ini", b"[Hooks]\nHookEngineTick = 0\n")])

    def test_requires_exact_build_and_existing_profile(self):
        for digest, profile in [("*", "Test Game"), ("a" * 64, "Missing")]:
            self.builds([{"executable": "Game.exe", "sha256": digest, "profile": profile}])
            with self.assertRaises(ValueError):
                embed.generate(self.root, self.manifest)

    def test_rejects_ambiguous_builds(self):
        self.builds([{"executable": name, "sha256": "a" * 64, "profile": "Test Game"} for name in ("Game.exe", "GAME.EXE")])
        with self.assertRaises(ValueError):
            embed.generate(self.root, self.manifest)

    def test_rejects_executable_paths(self):
        self.builds([{"executable": "../Game.exe", "sha256": "a" * 64, "profile": "Test Game"}])
        with self.assertRaises(ValueError):
            embed.generate(self.root, self.manifest)

    def test_rejects_nul_in_lua(self):
        (self.game / "Bad.lua").write_bytes(b"return 1\0return 2")
        with self.assertRaises(ValueError):
            embed.generate(self.root, self.manifest)

    def test_output_is_deterministic_and_contains_validated_build(self):
        self.builds([{"executable": "Game.exe", "sha256": "A" * 64, "profile": "Test Game"}])
        first = embed.generate(self.root, self.manifest)
        self.assertEqual(first, embed.generate(self.root, self.manifest))
        self.assertIn('"' + 'a' * 64 + '"', first)


if __name__ == "__main__":
    unittest.main()
