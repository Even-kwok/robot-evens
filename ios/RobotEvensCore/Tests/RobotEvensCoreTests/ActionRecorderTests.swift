import XCTest
@testable import RobotEvensCore

final class ActionRecorderTests: XCTestCase {
    func testRecorderBuildsActionWithStartSoundFaceEventsAndCompressedPoses() throws {
        var recorder = ActionRecorder(configuration: .test)
        recorder.start(id: "action.test_dance", title: "Test Dance", atMs: 1000)
        recorder.setStartSoundAssetId("sound.test_dance")
        recorder.recordPose(ServoPose(yaw: 0, pitch: 120, speed: 260), atMs: 1000)
        recorder.recordPose(ServoPose(yaw: 1, pitch: 121, speed: 260), atMs: 1010)
        recorder.recordPose(ServoPose(yaw: 100, pitch: 180, speed: 260), atMs: 1030)
        recorder.recordPose(ServoPose(yaw: 200, pitch: 220, speed: 260), atMs: 1060)
        recorder.recordFaceEvent(mood: "happy", atMs: 1070)
        recorder.recordPose(ServoPose(yaw: 201, pitch: 221, speed: 260), atMs: 1080)
        recorder.stop(atMs: 1080)

        let action = try recorder.buildAction(endMode: .home)

        XCTAssertEqual(action.id, "action.test_dance")
        XCTAssertEqual(action.title, "Test Dance")
        XCTAssertEqual(action.durationMs, 80)
        XCTAssertEqual(action.startSoundAssetId, "sound.test_dance")
        XCTAssertEqual(action.poseKeyframes, [
            PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260),
            PoseKeyframe(timeMs: 60, yaw: 200, pitch: 220, speed: 260),
            PoseKeyframe(timeMs: 80, yaw: 201, pitch: 221, speed: 260)
        ])
        XCTAssertEqual(action.faceEvents, [FaceEvent(timeMs: 70, mood: "happy")])
        XCTAssertNoThrow(try RobotActionValidator.validate(action))
    }

    func testPauseResumeRemovesPausedTimeAndIgnoresPausedEvents() throws {
        var recorder = ActionRecorder(configuration: .test)
        recorder.start(id: "action.pause_resume", title: "Pause Resume", atMs: 1000)
        recorder.recordPose(ServoPose(yaw: 0, pitch: 120, speed: 260), atMs: 1000)
        recorder.pause(atMs: 1050)
        recorder.recordFaceEvent(mood: "sad", atMs: 1100)
        recorder.resume(atMs: 2050)
        recorder.recordPose(ServoPose(yaw: 100, pitch: 180, speed: 260), atMs: 2060)
        recorder.stop(atMs: 2060)

        let action = try recorder.buildAction(endMode: .hold)

        XCTAssertEqual(action.durationMs, 60)
        XCTAssertEqual(action.faceEvents, [])
        XCTAssertEqual(action.poseKeyframes.map(\.timeMs), [0, 60])
        XCTAssertEqual(action.endMode, .hold)
    }
}

private extension ActionRecorderConfiguration {
    static let test = ActionRecorderConfiguration(
        minPoseSpacingMs: 50,
        minYawDelta: 10,
        minPitchDelta: 10
    )
}
