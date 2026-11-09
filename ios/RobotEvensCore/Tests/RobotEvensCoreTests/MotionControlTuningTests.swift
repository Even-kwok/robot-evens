import XCTest
@testable import RobotEvensCore

final class MotionControlTuningTests: XCTestCase {
    func testPrecisionTuningUsesFasterSamplingAndSmallerDeadZoneThanStandard() {
        XCTAssertEqual(MotionControlTuning.standard.motionUpdateHz, 30)
        XCTAssertEqual(MotionControlTuning.precision.motionUpdateHz, 60)
        XCTAssertEqual(MotionControlTuning.precision.liveSendIntervalMs, 16)
        XCTAssertLessThan(MotionControlTuning.precision.deadZoneRadians, MotionControlTuning.standard.deadZoneRadians)
        XCTAssertGreaterThan(MotionControlTuning.precision.smoothingAlpha, MotionControlTuning.standard.smoothingAlpha)
    }
}
