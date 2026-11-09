import json
import subprocess
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ASSET_ROOT = ROOT / "assets" / "eye-glyphs"
APPROVED_EYE_ROOT = ROOT / "assets" / "approved-eye-pairs"


class EyeGlyphAssetTests(unittest.TestCase):
    def test_generated_assets_are_current_and_cross_platform(self):
        result = subprocess.run(
            [sys.executable, "tools/generate_eye_glyph_assets.py", "--check"],
            cwd=ROOT,
            capture_output=True,
            text=True,
            timeout=60,
        )
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

        manifest = json.loads((ASSET_ROOT / "generated-manifest.json").read_text(encoding="utf-8"))
        self.assertEqual(manifest["glyphCount"], 64)
        self.assertEqual(manifest["width"], 192)
        self.assertEqual(manifest["height"], 192)
        self.assertEqual(manifest["bytesPerGlyph"], 4608)
        self.assertEqual(len(manifest["items"]), 64)
        self.assertEqual(len({item["id"] for item in manifest["items"]}), 64)
        for item in manifest["items"]:
            self.assertGreater(item["coverage"], 0.015, item["id"])
            self.assertLess(item["coverage"], 0.80, item["id"])
            self.assertTrue((ASSET_ROOT / item["source"]).is_file(), item["id"])

        stress_ids = {"fingerprint", "qrCode", "robot", "globe", "motorcycle", "settings"}
        self.assertTrue(stress_ids.issubset({item["id"] for item in manifest["items"]}))

        digest = manifest["catalogSHA256"]
        firmware = (ROOT / "firmware" / "eye_glyph_bitmaps.h").read_text(encoding="utf-8")
        swift = (
            ROOT
            / "ios"
            / "RobotEvensCore"
            / "Sources"
            / "RobotEvensCore"
            / "EyeGlyphBitmaps.swift"
        ).read_text(encoding="utf-8")
        self.assertIn(digest, firmware)
        self.assertIn(digest, swift)

        approved = json.loads(
            (APPROVED_EYE_ROOT / "generated-manifest.json").read_text(encoding="utf-8")
        )
        self.assertEqual(approved["presetCount"], 48)
        self.assertEqual(approved["eyeBitmapCount"], 96)
        self.assertEqual(approved["width"], 96)
        self.assertEqual(approved["height"], 96)
        self.assertEqual(approved["bytesPerEye"], 4608)
        self.assertEqual(len(approved["items"]), 48)
        self.assertEqual(len({item["id"] for item in approved["items"]}), 48)
        self.assertEqual(
            {item["batch"] for item in approved["items"]}, {"b1", "b2", "b3"}
        )
        for item in approved["items"]:
            self.assertGreaterEqual(item["recommendedEyeSpacing"], 48, item["id"])
            self.assertLessEqual(item["recommendedEyeSpacing"], 220, item["id"])
        approved_firmware = (
            ROOT / "firmware" / "approved_eye_pair_bitmaps.h"
        ).read_text(encoding="utf-8")
        approved_swift = (
            ROOT
            / "ios"
            / "RobotEvensCore"
            / "Sources"
            / "RobotEvensCore"
            / "ApprovedEyePairBitmaps.swift"
        ).read_text(encoding="utf-8")
        self.assertIn(approved["catalogSHA256"], approved_firmware)
        self.assertIn(approved["catalogSHA256"], approved_swift)
        self.assertIn("coverageAt", approved_firmware)
        self.assertIn("public static func coverage", approved_swift)

    def test_pinned_sources_include_attribution_and_license(self):
        catalog = json.loads((ASSET_ROOT / "catalog.json").read_text(encoding="utf-8"))
        self.assertEqual(catalog["source"]["license"], "Apache-2.0")
        self.assertEqual(len(catalog["source"]["commit"]), 40)
        self.assertTrue((ASSET_ROOT / "LICENSE").is_file())
        self.assertTrue((ASSET_ROOT / "ATTRIBUTION.md").is_file())
        self.assertTrue((ASSET_ROOT / "preview.png").is_file())
        self.assertTrue((ASSET_ROOT / "preview-eye-size.png").is_file())
        self.assertTrue((APPROVED_EYE_ROOT / "SOURCE.md").is_file())
        self.assertTrue((APPROVED_EYE_ROOT / "preview.png").is_file())
        self.assertTrue(
            (APPROVED_EYE_ROOT / "preview-runtime-320x240.png").is_file()
        )


if __name__ == "__main__":
    unittest.main()
