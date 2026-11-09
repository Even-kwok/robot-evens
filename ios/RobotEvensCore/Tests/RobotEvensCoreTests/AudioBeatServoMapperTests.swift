import XCTest
@testable import RobotEvensCore

final class AudioBeatServoMapperTests: XCTestCase {
    func testQuietLevelsHoldCenterPose() {
        var mapper = AudioBeatServoMapper(configuration: .test())

        let state = mapper.map(level: 0.08, atMs: 0)

        XCTAssertFalse(state.isBeat)
        XCTAssertEqual(state.pose, ServoPose(yaw: 0, pitch: 120, speed: 760))
    }

    func testLoudBeatRaisesPitchByConfiguredAmplitude() {
        var mapper = AudioBeatServoMapper(configuration: .test(pitchAmplitude: 150))

        let state = mapper.map(level: 0.9, atMs: 100)

        XCTAssertTrue(state.isBeat)
        XCTAssertEqual(state.pose.pitch, 270)
        XCTAssertEqual(state.pose.speed, 760)
    }

    func testYawShakeAlternatesLeftAndRightOnSeparateBeats() {
        var mapper = AudioBeatServoMapper(configuration: .test(
            pitchAmplitude: 0,
            yawAmplitude: 90,
            isYawEnabled: true,
            minimumBeatSpacingMs: 120
        ))

        let left = mapper.map(level: 0.9, atMs: 0)
        let right = mapper.map(level: 0.9, atMs: 160)

        XCTAssertTrue(left.isBeat)
        XCTAssertTrue(right.isBeat)
        XCTAssertEqual(left.pose.yaw, 90)
        XCTAssertEqual(right.pose.yaw, -90)
        XCTAssertEqual(left.pose.pitch, 120)
        XCTAssertEqual(right.pose.pitch, 120)
    }

    func testPitchAndYawAxesCanBeDisabledSeparately() {
        var mapper = AudioBeatServoMapper(configuration: .test(
            pitchAmplitude: 150,
            yawAmplitude: 80,
            isPitchEnabled: false,
            isYawEnabled: true
        ))

        let state = mapper.map(level: 0.9, atMs: 0)

        XCTAssertTrue(state.isBeat)
        XCTAssertEqual(state.pose.pitch, 120)
        XCTAssertEqual(state.pose.yaw, 80)
    }

    func testMotionCurveChangesReleaseShape() {
        var linear = AudioBeatServoMapper(configuration: .test(
            pitchAmplitude: 100,
            motionCurve: .linear,
            releaseMs: 1_000
        ))
        var smooth = AudioBeatServoMapper(configuration: .test(
            pitchAmplitude: 100,
            motionCurve: .easeInOut,
            releaseMs: 1_000
        ))

        _ = linear.map(level: 0.9, atMs: 0)
        _ = smooth.map(level: 0.9, atMs: 0)
        let linearQuarter = linear.map(level: 0.0, atMs: 250)
        let smoothQuarter = smooth.map(level: 0.0, atMs: 250)

        XCTAssertEqual(linearQuarter.pose.pitch, 195)
        XCTAssertEqual(smoothQuarter.pose.pitch, 205)
    }

    func testCooldownPreventsImmediateRetrigger() {
        var mapper = AudioBeatServoMapper(configuration: .test(minimumBeatSpacingMs: 180))

        _ = mapper.map(level: 0.9, atMs: 0)
        let blocked = mapper.map(level: 0.95, atMs: 90)
        let allowed = mapper.map(level: 0.95, atMs: 220)

        XCTAssertFalse(blocked.isBeat)
        XCTAssertTrue(allowed.isBeat)
    }

    func testPulseReleasesBackToCenterWhenMusicDrops() {
        var mapper = AudioBeatServoMapper(configuration: .test(releaseMs: 160))

        _ = mapper.map(level: 0.9, atMs: 0)
        let released = mapper.map(level: 0.0, atMs: 180)

        XCTAssertFalse(released.isBeat)
        XCTAssertEqual(released.pose.pitch, 120)
    }

    func testHigherSensitivityTriggersOnWeakerBeat() {
        var sensitive = AudioBeatServoMapper(configuration: .test(sensitivity: 0.9))
        var conservative = AudioBeatServoMapper(configuration: .test(sensitivity: 0.1))

        XCTAssertTrue(sensitive.map(level: 0.38, atMs: 0).isBeat)
        XCTAssertFalse(conservative.map(level: 0.38, atMs: 0).isBeat)
    }

    func testAdaptiveSpeedRaisesSpeedForFastBeatsAndLowersSpeedForSlowBeats() {
        var mapper = AudioBeatServoMapper(configuration: .test(
            speed: 650,
            isAdaptiveSpeedEnabled: true,
            adaptiveSlowSpeed: 300,
            adaptiveFastSpeed: 900,
            minimumBeatSpacingMs: 120
        ))

        let firstBeat = mapper.map(level: 0.9, atMs: 0)
        let fastBeat = mapper.map(level: 0.9, atMs: 220)
        let slowBeat = mapper.map(level: 0.9, atMs: 1_020)

        XCTAssertEqual(firstBeat.pose.speed, 650)
        XCTAssertEqual(fastBeat.pose.speed, 900)
        XCTAssertEqual(slowBeat.pose.speed, 300)
    }

    func testAdaptiveSpeedReturnsToBaseSpeedAfterPulseReleases() {
        var mapper = AudioBeatServoMapper(configuration: .test(
            speed: 650,
            isAdaptiveSpeedEnabled: true,
            adaptiveSlowSpeed: 300,
            adaptiveFastSpeed: 900,
            releaseMs: 160,
            minimumBeatSpacingMs: 120
        ))

        _ = mapper.map(level: 0.9, atMs: 0)
        _ = mapper.map(level: 0.9, atMs: 220)
        let released = mapper.map(level: 0, atMs: 400)

        XCTAssertEqual(released.pose.speed, 650)
    }

    func testRandomizedPitchAmplitudeSamplesOncePerBeatAndHoldsDuringRelease() {
        var mapper = AudioBeatServoMapper(configuration: .test(
            pitchAmplitude: 200,
            isPitchAmplitudeRandomized: true,
            releaseMs: 1_000,
            minimumBeatSpacingMs: 120
        ))

        let smallBeat = mapper.map(level: 1.0, atMs: 0, randomUnit: 0)
        let smallRelease = mapper.map(level: 0.0, atMs: 500, randomUnit: 1)
        let largeBeat = mapper.map(level: 1.0, atMs: 1_200, randomUnit: 1)

        XCTAssertTrue(smallBeat.isBeat)
        XCTAssertFalse(smallRelease.isBeat)
        XCTAssertTrue(largeBeat.isBeat)
        XCTAssertEqual(smallBeat.pose.pitch, 210)
        XCTAssertEqual(smallRelease.pose.pitch, 165)
        XCTAssertEqual(largeBeat.pose.pitch, 320)
    }

    func testRandomizedYawAmplitudeSamplesPerBeatWithoutChangingPitch() {
        var mapper = AudioBeatServoMapper(configuration: .test(
            pitchAmplitude: 200,
            yawAmplitude: 100,
            isPitchEnabled: false,
            isYawEnabled: true,
            isYawAmplitudeRandomized: true,
            minimumBeatSpacingMs: 120
        ))

        let smallLeft = mapper.map(level: 1.0, atMs: 0, randomUnit: 0)
        let largeRight = mapper.map(level: 1.0, atMs: 160, randomUnit: 1)

        XCTAssertEqual(smallLeft.pose.pitch, 120)
        XCTAssertEqual(largeRight.pose.pitch, 120)
        XCTAssertEqual(smallLeft.pose.yaw, 45)
        XCTAssertEqual(largeRight.pose.yaw, -100)
    }
}

private extension AudioBeatServoConfiguration {
    static func test(
        centerPitch: Int = 120,
        pitchAmplitude: Int = 120,
        yawAmplitude: Int = 0,
        isPitchEnabled: Bool = true,
        isYawEnabled: Bool = false,
        isPitchAmplitudeRandomized: Bool = false,
        isYawAmplitudeRandomized: Bool = false,
        motionCurve: RhythmMotionCurve = .linear,
        speed: Int = 760,
        isAdaptiveSpeedEnabled: Bool = false,
        adaptiveSlowSpeed: Int = 360,
        adaptiveFastSpeed: Int = 960,
        sensitivity: Double = 0.65,
        releaseMs: Int = 180,
        minimumBeatSpacingMs: Int = 140
    ) -> AudioBeatServoConfiguration {
        AudioBeatServoConfiguration(
            centerYaw: 0,
            centerPitch: centerPitch,
            pitchAmplitude: pitchAmplitude,
            yawAmplitude: yawAmplitude,
            isPitchEnabled: isPitchEnabled,
            isYawEnabled: isYawEnabled,
            isPitchAmplitudeRandomized: isPitchAmplitudeRandomized,
            isYawAmplitudeRandomized: isYawAmplitudeRandomized,
            motionCurve: motionCurve,
            speed: speed,
            isAdaptiveSpeedEnabled: isAdaptiveSpeedEnabled,
            adaptiveSlowSpeed: adaptiveSlowSpeed,
            adaptiveFastSpeed: adaptiveFastSpeed,
            sensitivity: sensitivity,
            releaseMs: releaseMs,
            minimumBeatSpacingMs: minimumBeatSpacingMs
        )
    }
}
