import XCTest
@testable import RobotEvensCore

final class MotionMapperTests: XCTestCase {
    func testCalibrationMakesCurrentAttitudeNeutral() {
        var mapper = MotionMapper(configuration: .test(smoothingAlpha: 1.0))
        let neutral = PhoneAttitude(rollRadians: 0.2, pitchRadians: -0.1)

        mapper.calibrate(neutral)
        let pose = mapper.map(neutral)

        XCTAssertEqual(pose, ServoPose(yaw: 0, pitch: 120, speed: 260))
    }

    func testPositiveRollMapsToPositiveYaw() {
        var mapper = MotionMapper(configuration: .test(smoothingAlpha: 1.0))
        mapper.calibrate(.zero)

        let pose = mapper.map(PhoneAttitude(rollRadians: 0.5, pitchRadians: 0))

        XCTAssertEqual(pose.yaw, 640)
        XCTAssertEqual(pose.pitch, 120)
    }

    func testYawAndPitchClampToFirmwareRange() {
        var mapper = MotionMapper(configuration: .test(smoothingAlpha: 1.0))
        mapper.calibrate(.zero)

        let highPose = mapper.map(PhoneAttitude(rollRadians: 10, pitchRadians: 10))
        let lowPose = mapper.map(PhoneAttitude(rollRadians: -10, pitchRadians: -10))

        XCTAssertEqual(highPose.yaw, 1280)
        XCTAssertEqual(highPose.pitch, 900)
        XCTAssertEqual(lowPose.yaw, -1280)
        XCTAssertEqual(lowPose.pitch, 0)
    }

    func testDeadZoneSuppressesTinyMotion() {
        var mapper = MotionMapper(configuration: .test(deadZoneRadians: 0.05, smoothingAlpha: 1.0))
        mapper.calibrate(.zero)

        let pose = mapper.map(PhoneAttitude(rollRadians: 0.01, pitchRadians: 0.01))

        XCTAssertEqual(pose, ServoPose(yaw: 0, pitch: 120, speed: 260))
    }

    func testSmoothingMovesPartwayTowardTarget() {
        var mapper = MotionMapper(configuration: .test(smoothingAlpha: 0.5))
        mapper.calibrate(.zero)

        let pose = mapper.map(PhoneAttitude(rollRadians: 1.0, pitchRadians: 0))

        XCTAssertEqual(pose.yaw, 640)
        XCTAssertEqual(pose.pitch, 120)
    }

    func testSetSpeedUpdatesMappedPoseSpeed() {
        var mapper = MotionMapper(configuration: .test(smoothingAlpha: 1.0))

        mapper.setSpeed(820)
        let pose = mapper.map(.zero)

        XCTAssertEqual(pose.speed, 820)
    }

    func testSetSpeedClampsToFirmwareRange() {
        var mapper = MotionMapper(configuration: .test(smoothingAlpha: 1.0))

        mapper.setSpeed(1200)
        XCTAssertEqual(mapper.map(.zero).speed, 1000)

        mapper.setSpeed(-40)
        XCTAssertEqual(mapper.map(.zero).speed, 0)
    }
}

private extension MotionMapperConfiguration {
    static func test(deadZoneRadians: Double = 0, smoothingAlpha: Double) -> MotionMapperConfiguration {
        MotionMapperConfiguration(
            maxRollRadians: 1.0,
            maxPitchRadians: 1.0,
            deadZoneRadians: deadZoneRadians,
            smoothingAlpha: smoothingAlpha,
            restPitch: 120,
            speed: 260
        )
    }
}
