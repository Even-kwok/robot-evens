import XCTest
@testable import RobotEvensCore

final class CreatorModelsTests: XCTestCase {
    func testApprovedSolidEyePresetCatalogHasThreeFullPagesAndIsFirmwareBounded() {
        XCTAssertEqual(SolidEyePreset.allCases.count, 48)
        XCTAssertEqual(Set(SolidEyePreset.allCases.map(\.collection)), Set(SolidEyePresetCollection.allCases))

        for preset in SolidEyePreset.allCases {
            XCTAssertFalse(preset.title.isEmpty, preset.rawValue)
            XCTAssertFalse(preset.emotionHint.isEmpty, preset.rawValue)
            XCTAssertTrue(ApprovedEyePairBitmapCatalog.contains(preset.glyphId), preset.rawValue)
            XCTAssertTrue((0.35...2.2).contains(preset.recommendedWidthScale), preset.rawValue)
            XCTAssertTrue((0.15...2.2).contains(preset.recommendedHeightScale), preset.rawValue)
            XCTAssertTrue((48...220).contains(preset.recommendedEyeSpacing), preset.rawValue)
            XCTAssertLessThanOrEqual(
                abs(Double(preset.recommendedEyeSpacing) - 72 * preset.recommendedWidthScale),
                1,
                preset.rawValue
            )
        }
        XCTAssertEqual(ApprovedEyePairBitmapCatalog.ids, SolidEyePreset.allCases.map(\.rawValue))
    }

    func testApprovedEyePairsKeepTheirOriginalLeftRightAsymmetry() throws {
        XCTAssertEqual(ApprovedEyePairBitmapCatalog.width, 96)
        XCTAssertEqual(ApprovedEyePairBitmapCatalog.height, 96)
        let asymmetricIDs = ["approved.b1.16", "approved.b2.02", "approved.b2.04", "approved.b3.16"]
        for id in asymmetricIDs {
            let left = try XCTUnwrap(ApprovedEyePairBitmapCatalog.bitmap(for: id, side: .left))
            let right = try XCTUnwrap(ApprovedEyePairBitmapCatalog.bitmap(for: id, side: .right))
            XCTAssertEqual(left.count, ApprovedEyePairBitmapCatalog.bytesPerEye)
            XCTAssertEqual(right.count, ApprovedEyePairBitmapCatalog.bytesPerEye)
            XCTAssertNotEqual(left, right, id)
            XCTAssertTrue(left.contains { $0 != 0 }, id)
            XCTAssertTrue(right.contains { $0 != 0 }, id)
            XCTAssertTrue((0...15).contains(ApprovedEyePairBitmapCatalog.coverage(
                in: left,
                x: ApprovedEyePairBitmapCatalog.width / 2,
                y: ApprovedEyePairBitmapCatalog.height / 2
            )), id)
        }
    }

    func testGlyphEyeCatalogCoversSimpleAndStressDetailCategories() throws {
        XCTAssertEqual(EyeGlyphPreset.allCases.count, 64)
        XCTAssertEqual(Set(EyeGlyphPreset.allCases.map(\.category)), Set(EyeGlyphCategory.allCases))
        XCTAssertEqual(Set(EyeGlyphPreset.allCases.map(\.rawValue)).count, EyeGlyphPreset.allCases.count)

        for glyph in EyeGlyphPreset.allCases {
            XCTAssertFalse(glyph.title.isEmpty, glyph.rawValue)
            XCTAssertTrue((0.35...2.2).contains(glyph.recommendedWidthScale), glyph.rawValue)
            XCTAssertTrue((0.15...2.2).contains(glyph.recommendedHeightScale), glyph.rawValue)
        }

        XCTAssertEqual(EyeGlyphBitmapCatalog.width, 192)
        XCTAssertEqual(EyeGlyphBitmapCatalog.height, 192)
        XCTAssertEqual(
            EyeGlyphBitmapCatalog.ids,
            EyeGlyphPreset.allCases.map(\.rawValue)
        )
        for glyph in EyeGlyphPreset.allCases {
            let bitmap = try XCTUnwrap(EyeGlyphBitmapCatalog.bitmap(for: glyph.rawValue))
            XCTAssertEqual(bitmap.count, EyeGlyphBitmapCatalog.bytesPerGlyph)
            XCTAssertTrue(bitmap.contains { $0 != 0 }, glyph.rawValue)
        }
        XCTAssertNil(EyeGlyphBitmapCatalog.bitmap(for: "unknown"))

        let design = EyeShapeDesign(glyph: .basketball)
        let decoded = try JSONDecoder().decode(EyeShapeDesign.self, from: design.encoded())
        XCTAssertEqual(decoded, design)
        XCTAssertTrue(decoded.isGlyph)
        XCTAssertEqual(decoded.glyphId, "basketball")
        XCTAssertTrue(decoded.points.isEmpty)
    }

    func testEyeShapeDesignRoundTripsAsVersionedJSON() throws {
        var design = EyeShapeDesign(
            name: "测试眼形",
            baseMood: "angry",
            sweepEnabled: true,
            points: [
                NormalizedPoint(x: 0.05, y: 0.2),
                NormalizedPoint(x: 0.55, y: 0.05),
                NormalizedPoint(x: 0.95, y: 0.4),
                NormalizedPoint(x: 0.72, y: 0.92),
                NormalizedPoint(x: 0.16, y: 0.8)
            ],
            smoothingEnabled: true,
            smoothPoints: [false, true, true, false, false],
            style: RobotFaceStyle(eyeWidthScale: 1.3, eyeColor: "#ff3300")
        )
        design.setLinkedHandle(EyeHandleOffset(x: 0.24, y: -0.08), incoming: false, at: 1)

        let data = try design.encoded()
        let decoded = try JSONDecoder().decode(EyeShapeDesign.self, from: data)

        XCTAssertEqual(decoded, design)
        XCTAssertEqual(decoded.schemaVersion, 5)
        XCTAssertTrue(decoded.sweepEnabled)
        XCTAssertTrue(decoded.isPointSmooth(at: 1))
        XCTAssertFalse(decoded.isPointSmooth(at: 0))
        XCTAssertEqual(decoded.controlHandles[1]?.outgoing, EyeHandleOffset(x: 0.24, y: -0.08))
        XCTAssertEqual(decoded.controlHandles[1]?.incoming, EyeHandleOffset(x: -0.24, y: 0.08))
    }

    func testRobotEyeLibrarySnapshotDecodesMoodOverrideAndBezierHandles() throws {
        let payload = Data(#"""
        {
          "ok": true,
          "revision": 4,
          "activeId": "system.angry",
          "customCapacity": 15,
          "entries": [{
            "id": "system.angry",
            "kind": "system",
            "name": "angry",
            "baseMood": "angry",
            "hasOverride": true,
            "deletable": true,
            "editableShape": true,
            "overrideName": "生气替换",
            "sweepEnabled": true,
            "mirrorRightEye": true,
            "smoothingEnabled": true,
            "points": [
              {"x":0.1,"y":0.2,"smooth":false},
              {"x":0.5,"y":0.0,"smooth":true,"handleInX":-0.2,"handleInY":0.1,"handleOutX":0.2,"handleOutY":-0.1},
              {"x":0.9,"y":0.8,"smooth":false}
            ],
            "style": {
              "eyeWidthScale":1.2,"eyeHeightScale":1.0,"cornerRadius":24,
              "eyeOffsetX":0,"eyeOffsetY":0,"eyeSpacing":112,"eyeColor":"#ff3300",
              "glowEnabled":false,"glowStrength":3,"scanlinesEnabled":true,
              "scanlineSpacing":5,"scanlineAlpha":20
            }
          }]
        }
        """#.utf8)

        let snapshot = try JSONDecoder().decode(RobotEyeLibrarySnapshot.self, from: payload)

        XCTAssertTrue(snapshot.ok)
        XCTAssertEqual(snapshot.revision, 4)
        XCTAssertEqual(snapshot.entries.first?.hasOverride, true)
        XCTAssertEqual(snapshot.entries.first?.overrideName, "生气替换")
        XCTAssertEqual(snapshot.entries.first?.sweepEnabled, true)
        XCTAssertEqual(snapshot.entries.first?.design?.sweepEnabled, true)
        XCTAssertEqual(snapshot.entries.first?.design?.baseMood, "angry")
        XCTAssertEqual(
            snapshot.entries.first?.design?.controlHandles[1]?.outgoing,
            EyeHandleOffset(x: 0.2, y: -0.1)
        )
    }

    func testRobotEyeLibrarySnapshotDecodesGlyphOverrideWithoutPathPoints() throws {
        let payload = Data(#"""
        {
          "ok": true,
          "revision": 8,
          "activeId": "system.happy",
          "customCapacity": 15,
          "entries": [{
            "id":"system.happy","kind":"system","name":"happy","baseMood":"happy",
            "hasOverride":true,"deletable":true,"editableShape":false,
            "overrideName":"篮球符号眼","glyphId":"basketball","points":[],
            "style":{"eyeWidthScale":1,"eyeHeightScale":1,"cornerRadius":24,
              "eyeOffsetX":0,"eyeOffsetY":-35,"eyeSpacing":112,"eyeColor":"#00f2ff",
              "glowEnabled":false,"glowStrength":3,"scanlinesEnabled":true,
              "scanlineSpacing":5,"scanlineAlpha":20}
          }]
        }
        """#.utf8)

        let snapshot = try JSONDecoder().decode(RobotEyeLibrarySnapshot.self, from: payload)
        let entry = try XCTUnwrap(snapshot.entries.first)
        XCTAssertEqual(entry.glyphId, "basketball")
        XCTAssertEqual(entry.design?.glyphId, "basketball")
        XCTAssertTrue(entry.design?.isGlyph == true)
        XCTAssertTrue(entry.design?.points.isEmpty == true)
    }

    func testVersionOneEyeShapeMigratesWithoutChangingAngularOutline() throws {
        let legacy = Data(#"""
        {
          "schemaVersion": 1,
          "name": "旧眼形",
          "baseMood": "angry",
          "mirrorRightEye": true,
          "points": [{"x":0.0,"y":0.0},{"x":1.0,"y":0.0},{"x":0.5,"y":1.0}]
        }
        """#.utf8)

        let decoded = try JSONDecoder().decode(EyeShapeDesign.self, from: legacy)

        XCTAssertEqual(decoded.schemaVersion, 5)
        XCTAssertFalse(decoded.sweepEnabled)
        XCTAssertFalse(decoded.smoothingEnabled)
        XCTAssertEqual(decoded.smoothPoints, [false, false, false])
        XCTAssertEqual(decoded.controlHandles.count, 3)
        XCTAssertTrue(decoded.controlHandles.allSatisfy { $0 == nil })
    }

    func testVersionTwoEyeShapeMigratesToAutomaticHandles() throws {
        let legacy = Data(#"""
        {
          "schemaVersion": 2,
          "name": "旧曲线眼形",
          "points": [{"x":0.0,"y":0.4},{"x":0.5,"y":0.0},{"x":1.0,"y":0.4}],
          "smoothingEnabled": true,
          "smoothPoints": [true,true,true]
        }
        """#.utf8)

        let decoded = try JSONDecoder().decode(EyeShapeDesign.self, from: legacy)

        XCTAssertEqual(decoded.schemaVersion, 5)
        XCTAssertFalse(decoded.sweepEnabled)
        XCTAssertTrue(decoded.controlHandles.allSatisfy { $0 == nil })
        XCTAssertNotNil(decoded.effectiveCurveHandles(at: 1))
    }

    func testDraggingEitherBezierHandleKeepsTheOppositeSideLinked() {
        var design = EyeShapeDesign(
            points: [
                NormalizedPoint(x: 0.05, y: 0.5),
                NormalizedPoint(x: 0.30, y: 0.1),
                NormalizedPoint(x: 0.70, y: 0.1),
                NormalizedPoint(x: 0.95, y: 0.5),
                NormalizedPoint(x: 0.50, y: 0.9)
            ],
            smoothingEnabled: true
        )

        design.setLinkedHandle(EyeHandleOffset(x: -0.31, y: 0.12), incoming: true, at: 2)

        XCTAssertEqual(design.controlHandles[2]?.incoming, EyeHandleOffset(x: -0.31, y: 0.12))
        XCTAssertEqual(design.controlHandles[2]?.outgoing, EyeHandleOffset(x: 0.31, y: -0.12))
        design.resetHandles(at: 2)
        XCTAssertNil(design.controlHandles[2])
        XCTAssertNotNil(design.effectiveCurveHandles(at: 2))
    }

    func testApprovedPresetUsesSharedBitmapInsteadOfInventedPathGeometry() {
        let design = EyeShapeDesign(preset: .batch3_06)

        XCTAssertEqual(design.glyphId, "approved.b3.06")
        XCTAssertTrue(design.isGlyph)
        XCTAssertTrue(design.points.isEmpty)
        XCTAssertFalse(design.smoothingEnabled)
        XCTAssertEqual(
            design.style.eyeSpacing,
            ApprovedEyePairBitmapCatalog.recommendedEyeSpacing(for: "approved.b3.06")
        )
    }

    func testEmotionSynthesizerIsDeterministicAndUsesFirmwareSampleRate() {
        let first = EmotionSoundProcessor.synthesize(slot: .angry, durationMs: 500)
        let second = EmotionSoundProcessor.synthesize(slot: .angry, durationMs: 500)

        XCTAssertEqual(first, second)
        XCTAssertEqual(first.count, 8_000)
        XCTAssertGreaterThan(first.map { abs($0) }.max() ?? 0, 0.2)
    }

    func testSpeedChangesDurationWithoutChangingFirmwareFormat() {
        let source = EmotionSoundProcessor.synthesize(slot: .happy, durationMs: 1_000)
        let processed = EmotionSoundProcessor.process(
            source,
            settings: EmotionSoundSettings(speed: 2, pitchSemitones: 5, robotAmount: 0.7)
        )
        let raw = EmotionSoundProcessor.rawU8Data(from: processed)

        XCTAssertEqual(processed.count, 8_000)
        XCTAssertEqual(raw.count, 8_000)
    }

    func testRawSilenceAndWavHeaderMatchFirmwareFormat() {
        let raw = EmotionSoundProcessor.rawU8Data(from: [0, 0, 0])
        let wav = EmotionSoundProcessor.wavData(fromRawU8: raw)

        XCTAssertEqual(raw, Data([128, 128, 128]))
        XCTAssertEqual(String(data: wav.prefix(4), encoding: .ascii), "RIFF")
        XCTAssertEqual(String(data: wav[8..<12], encoding: .ascii), "WAVE")
        XCTAssertEqual(wav.count, 47)
    }

    func testSoundOutputIsCappedAtFirmwareTwentySecondBudget() {
        let oversized = [Float](repeating: 0.2, count: EmotionSoundProcessor.maximumSampleCount * 2)

        XCTAssertEqual(
            EmotionSoundProcessor.rawU8Data(from: oversized).count,
            EmotionSoundProcessor.maximumSampleCount
        )
    }
}
