import XCTest
@testable import RobotEvensCore

final class ManualPoseEditorTests: XCTestCase {
    func testSetYawPitchAndSpeedUpdatesPose() {
        var editor = ManualPoseEditor()

        editor.setYaw(420)
        editor.setPitch(260)
        editor.setSpeed(840)

        XCTAssertEqual(editor.pose, ServoPose(yaw: 420, pitch: 260, speed: 840))
    }

    func testManualPoseClampsToFirmwareRange() {
        var editor = ManualPoseEditor()

        editor.setYaw(2000)
        editor.setPitch(-50)
        editor.setSpeed(1200)

        XCTAssertEqual(editor.pose, ServoPose(yaw: 1280, pitch: 0, speed: 1000))
    }

    func testEditorCanStartFromCurrentPose() {
        let editor = ManualPoseEditor(pose: ServoPose(yaw: -300, pitch: 450, speed: 620))

        XCTAssertEqual(editor.pose, ServoPose(yaw: -300, pitch: 450, speed: 620))
    }
}
