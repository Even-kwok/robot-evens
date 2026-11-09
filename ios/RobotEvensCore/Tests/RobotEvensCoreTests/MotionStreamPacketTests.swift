import Foundation
import XCTest
@testable import RobotEvensCore

final class MotionStreamPacketTests: XCTestCase {
    func testPacketEncodesCompactBigEndianPoseFrame() {
        let packet = MotionStreamPacket(sequence: 0x1234, pose: ServoPose(yaw: -12, pitch: 345, speed: 1000))

        XCTAssertEqual(
            packet.encoded(),
            Data([0x52, 0x45, 0x01, 0x00, 0x12, 0x34, 0xFF, 0xF4, 0x01, 0x59, 0x03, 0xE8])
        )
    }

    func testPacketClampsPoseToFirmwareRangeBeforeEncoding() {
        let packet = MotionStreamPacket(sequence: 7, pose: ServoPose(yaw: 2000, pitch: -50, speed: 1400))

        XCTAssertEqual(
            packet.encoded(),
            Data([0x52, 0x45, 0x01, 0x00, 0x00, 0x07, 0x05, 0x00, 0x00, 0x00, 0x03, 0xE8])
        )
    }
}
