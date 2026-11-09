import XCTest
@testable import RobotEvensCore

final class PlaybackSchedulerTests: XCTestCase {
    func testCommandsIncludeStartSoundTracksAndFinalHomeInTimeOrder() {
        let action = RobotAction(
            id: "action.playback",
            title: "Playback",
            durationMs: 200,
            startSoundAssetId: "sound.playback",
            endMode: .home,
            poseKeyframes: [
                PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260),
                PoseKeyframe(timeMs: 100, yaw: 120, pitch: 180, speed: 300)
            ],
            faceEvents: [
                FaceEvent(timeMs: 50, mood: "happy")
            ]
        )

        let commands = PlaybackScheduler.commands(for: action)

        XCTAssertEqual(commands.map(\.timeMs), [0, 0, 50, 100, 200])
        XCTAssertEqual(commands[0].request, RobotEndpointBuilder.invokeAsset("sound.playback"))
        XCTAssertEqual(commands[1].request, RobotEndpointBuilder.pose(ServoPose(yaw: 0, pitch: 120, speed: 260)))
        XCTAssertEqual(commands[2].request, RobotEndpointBuilder.face(mood: "happy"))
        XCTAssertEqual(commands[3].request, RobotEndpointBuilder.pose(ServoPose(yaw: 120, pitch: 180, speed: 300)))
        XCTAssertEqual(commands[4].request, RobotEndpointBuilder.home())
    }

    func testHoldEndModeDoesNotAppendHomeCommand() {
        let action = RobotAction(
            id: "action.hold",
            title: "Hold",
            durationMs: 100,
            endMode: .hold,
            poseKeyframes: [PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260)],
            faceEvents: []
        )

        let commands = PlaybackScheduler.commands(for: action)

        XCTAssertEqual(commands.map(\.timeMs), [0])
        XCTAssertEqual(commands[0].request, RobotEndpointBuilder.pose(ServoPose(yaw: 0, pitch: 120, speed: 260)))
    }

    func testCommandsIncludeTimedSoundEventsInTimeOrder() {
        let action = RobotAction(
            id: "action.timed_sounds",
            title: "Timed Sounds",
            durationMs: 800,
            soundEvents: [
                SoundEvent(timeMs: 300, assetId: "sound.tick"),
                SoundEvent(timeMs: 0, assetId: "sound.start")
            ],
            endMode: .hold,
            poseKeyframes: [
                PoseKeyframe(timeMs: 0, yaw: 0, pitch: 120, speed: 260),
                PoseKeyframe(timeMs: 300, yaw: 80, pitch: 140, speed: 300)
            ],
            faceEvents: []
        )

        let commands = PlaybackScheduler.commands(for: action)

        XCTAssertEqual(commands.map(\.timeMs), [0, 0, 300, 300])
        XCTAssertEqual(commands[0].request, RobotEndpointBuilder.invokeAsset("sound.start"))
        XCTAssertEqual(commands[1].request, RobotEndpointBuilder.pose(ServoPose(yaw: 0, pitch: 120, speed: 260)))
        XCTAssertEqual(commands[2].request, RobotEndpointBuilder.invokeAsset("sound.tick"))
        XCTAssertEqual(commands[3].request, RobotEndpointBuilder.pose(ServoPose(yaw: 80, pitch: 140, speed: 300)))
    }
}
