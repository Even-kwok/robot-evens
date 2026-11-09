import XCTest
@testable import RobotEvensCore

final class RobotActionTests: XCTestCase {
    func testCompactActionEncodesStorageKeys() throws {
        let action = RobotAction(
            id: "action.cute_dance_01",
            title: "Cute Dance 01",
            durationMs: 8500,
            startSoundAssetId: "sound.cute_dance_01",
            endMode: .home,
            poseKeyframes: [
                PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260),
                PoseKeyframe(timeMs: 320, yaw: 180, pitch: 170, speed: 320)
            ],
            faceEvents: [
                FaceEvent(timeMs: 0, mood: "happy"),
                FaceEvent(timeMs: 2400, mood: "love")
            ]
        )

        let data = try CompactActionCodec.encode(action)
        let object = try JSONSerialization.jsonObject(with: data) as? [String: Any]

        XCTAssertEqual(object?["v"] as? Int, 1)
        XCTAssertEqual(object?["id"] as? String, "action.cute_dance_01")
        XCTAssertEqual(object?["dur"] as? Int, 8500)
        XCTAssertEqual(object?["sound"] as? String, "sound.cute_dance_01")
        XCTAssertEqual(object?["end"] as? String, "home")
        XCTAssertEqual((object?["poses"] as? [[Any]])?.count, 2)
        XCTAssertEqual((object?["faces"] as? [[Any]])?.count, 2)
    }

    func testCompactActionEncodesTimedSoundEvents() throws {
        let action = RobotAction(
            id: "action.sound_timeline",
            title: "Sound Timeline",
            durationMs: 1200,
            soundEvents: [
                SoundEvent(timeMs: 0, assetId: "sound.beat_1"),
                SoundEvent(timeMs: 640, assetId: "sound.beat_2")
            ],
            endMode: .home,
            poseKeyframes: [PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260)],
            faceEvents: []
        )

        let data = try CompactActionCodec.encode(action)
        let object = try JSONSerialization.jsonObject(with: data) as? [String: Any]
        let sounds = object?["sounds"] as? [[Any]]

        XCTAssertNil(object?["sound"])
        XCTAssertEqual(sounds?.count, 2)
        XCTAssertEqual(sounds?[0][0] as? Int, 0)
        XCTAssertEqual(sounds?[0][1] as? String, "sound.beat_1")
        XCTAssertEqual(sounds?[1][0] as? Int, 640)
        XCTAssertEqual(sounds?[1][1] as? String, "sound.beat_2")
    }

    func testValidatorRejectsEmptyActionId() {
        var action = Self.validAction()
        action = RobotAction(
            id: "",
            title: action.title,
            durationMs: action.durationMs,
            startSoundAssetId: action.startSoundAssetId,
            endMode: action.endMode,
            poseKeyframes: action.poseKeyframes,
            faceEvents: action.faceEvents
        )

        XCTAssertThrowsError(try RobotActionValidator.validate(action)) { error in
            XCTAssertEqual(error as? RobotActionValidationError, .invalidId)
        }
    }

    func testValidatorRejectsDurationAboveThirtySeconds() {
        let action = RobotAction(
            id: "action.too_long",
            title: "Too Long",
            durationMs: 30001,
            endMode: .home,
            poseKeyframes: [PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260)],
            faceEvents: []
        )

        XCTAssertThrowsError(try RobotActionValidator.validate(action)) { error in
            XCTAssertEqual(error as? RobotActionValidationError, .durationOutOfRange)
        }
    }

    func testValidatorRejectsMoreThanTwoHundredFiftySixPoseKeyframes() {
        let poses = (0...256).map { index in
            PoseKeyframe(timeMs: index * 50, yaw: 0, pitch: 120, speed: 260)
        }
        let action = RobotAction(
            id: "action.too_many_poses",
            title: "Too Many Poses",
            durationMs: 30000,
            endMode: .home,
            poseKeyframes: poses,
            faceEvents: []
        )

        XCTAssertThrowsError(try RobotActionValidator.validate(action)) { error in
            XCTAssertEqual(error as? RobotActionValidationError, .tooManyPoseKeyframes)
        }
    }

    func testValidatorRejectsMoreThanSixtyFourFaceEvents() {
        let faces = (0...64).map { index in
            FaceEvent(timeMs: index * 100, mood: "happy")
        }
        let action = RobotAction(
            id: "action.too_many_faces",
            title: "Too Many Faces",
            durationMs: 10000,
            endMode: .home,
            poseKeyframes: [PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260)],
            faceEvents: faces
        )

        XCTAssertThrowsError(try RobotActionValidator.validate(action)) { error in
            XCTAssertEqual(error as? RobotActionValidationError, .tooManyFaceEvents)
        }
    }

    func testValidatorRejectsSoundEventsOutsideDuration() {
        let action = RobotAction(
            id: "action.sound_outside_duration",
            title: "Sound Outside Duration",
            durationMs: 1000,
            soundEvents: [SoundEvent(timeMs: 1200, assetId: "sound.late")],
            endMode: .home,
            poseKeyframes: [PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260)],
            faceEvents: []
        )

        XCTAssertThrowsError(try RobotActionValidator.validate(action)) { error in
            XCTAssertEqual(error as? RobotActionValidationError, .soundTimeOutOfRange)
        }
    }

    func testValidatorRejectsEncodedJsonAboveThirtyTwoKilobytes() {
        let action = RobotAction(
            id: "action.large",
            title: String(repeating: "A", count: 33000),
            durationMs: 1000,
            endMode: .home,
            poseKeyframes: [PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260)],
            faceEvents: []
        )

        XCTAssertThrowsError(try RobotActionValidator.validate(action)) { error in
            XCTAssertEqual(error as? RobotActionValidationError, .encodedSizeTooLarge)
        }
    }

    private static func validAction() -> RobotAction {
        RobotAction(
            id: "action.valid",
            title: "Valid",
            durationMs: 1000,
            startSoundAssetId: "sound.valid",
            endMode: .home,
            poseKeyframes: [PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260)],
            faceEvents: [FaceEvent(timeMs: 0, mood: "happy")]
        )
    }
}
