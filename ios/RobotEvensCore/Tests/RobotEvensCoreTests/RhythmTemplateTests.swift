import XCTest
@testable import RobotEvensCore

final class RhythmTemplateTests: XCTestCase {
    func testBuiltInRhythmTemplatesExposeRequestedStarterTemplates() {
        let templates = BuiltInRhythmTemplateLibrary.templates

        XCTAssertEqual(templates.map(\.id), [
            "rhythm.micro_fast_head_shake",
            "rhythm.micro_flash_nod",
            "rhythm.rhythmic_vertical_nod",
            "rhythm.rhythmic_side_nod",
            "rhythm.normal_head_shake",
            "rhythm.normal_vertical_nod"
        ])
        XCTAssertTrue(templates.allSatisfy { $0.cycleMs > 0 })
        XCTAssertTrue(templates.allSatisfy { (0...1000).contains($0.speed) })

        let microShake = templates[0]
        XCTAssertLessThanOrEqual(microShake.yawAmplitude, 90)

        let normalShake = templates[4]
        XCTAssertGreaterThanOrEqual(normalShake.yawAmplitude, 300)
    }

    func testTemplateSamplingUsesEditableNumericParameters() {
        let template = RhythmMotionTemplate(
            id: "rhythm.test",
            title: "Test",
            centerYaw: 10,
            centerPitch: 120,
            yawAmplitude: 100,
            pitchAmplitude: 40,
            cycleMs: 1_000,
            yawPhaseDegrees: 0,
            pitchPhaseDegrees: 90,
            speed: 720,
            curve: .easeInOut
        )

        let quarter = template.pose(atElapsedMs: 250)

        XCTAssertEqual(quarter.yaw, 110)
        XCTAssertEqual(quarter.pitch, 120)
        XCTAssertEqual(quarter.speed, 720)
    }

    func testMixerKeepsExistingTemplateRunningWhenAnotherStarts() {
        let yawTemplate = RhythmMotionTemplate(
            id: "rhythm.yaw",
            title: "Yaw",
            centerYaw: 0,
            centerPitch: 120,
            yawAmplitude: 100,
            pitchAmplitude: 0,
            cycleMs: 1_000,
            speed: 600,
            curve: .easeInOut
        )
        let pitchTemplate = RhythmMotionTemplate(
            id: "rhythm.pitch",
            title: "Pitch",
            centerYaw: 0,
            centerPitch: 120,
            yawAmplitude: 0,
            pitchAmplitude: 50,
            cycleMs: 1_000,
            speed: 800,
            curve: .easeInOut
        )

        let pose = RhythmTemplateMixer.pose(
            atElapsedMs: 450,
            activeLayers: [
                RhythmActiveTemplate(template: yawTemplate, startedAtMs: 0),
                RhythmActiveTemplate(template: pitchTemplate, startedAtMs: 200)
            ]
        )

        XCTAssertEqual(pose.yaw, 31)
        XCTAssertEqual(pose.pitch, 170)
        XCTAssertEqual(pose.speed, 800)
    }

    func testRecordedPerformanceRendersFullServoPoseStream() throws {
        let template = RhythmMotionTemplate(
            id: "rhythm.yaw",
            title: "Yaw",
            centerYaw: 0,
            centerPitch: 120,
            yawAmplitude: 100,
            pitchAmplitude: 0,
            cycleMs: 1_000,
            speed: 600,
            curve: .easeInOut
        )
        let action = try RhythmPerformanceRenderer.renderAction(
            id: "action.rhythm_test",
            title: "Rhythm Test",
            durationMs: 240,
            events: [
                RhythmTemplateEvent(timeMs: 0, templateId: template.id, isActive: true),
                RhythmTemplateEvent(timeMs: 180, templateId: template.id, isActive: false)
            ],
            templates: [template],
            sampleIntervalMs: 60
        )

        XCTAssertEqual(action.poseKeyframes.map(\.timeMs), [0, 60, 120, 180, 240])
        XCTAssertEqual(action.poseKeyframes[0].yaw, 0)
        XCTAssertNotEqual(action.poseKeyframes[1].yaw, 0)
        XCTAssertEqual(action.poseKeyframes[3].yaw, 0)
        XCTAssertEqual(action.poseKeyframes[4].yaw, 0)
        XCTAssertEqual(action.endMode, .home)
    }
}
