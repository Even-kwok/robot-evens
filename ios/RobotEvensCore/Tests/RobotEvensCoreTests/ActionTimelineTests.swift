import XCTest
@testable import RobotEvensCore

final class ActionTimelineTests: XCTestCase {
    func testBuiltInActionClipsExposeRequestedStarterPackages() throws {
        let clips = BuiltInActionClipLibrary.clips

        XCTAssertEqual(clips.map(\.id), [
            "clip.chat_soft_idle",
            "clip.micro_fast_head_shake",
            "clip.micro_flash_nod",
            "clip.rhythmic_vertical_nod",
            "clip.rhythmic_side_nod",
            "clip.normal_head_shake",
            "clip.normal_vertical_nod"
        ])
        XCTAssertTrue(clips.allSatisfy { $0.durationMs > 0 })
        XCTAssertTrue(clips.allSatisfy { !$0.poseKeyframes.isEmpty })

        let microShake = try XCTUnwrap(clips.first { $0.id == "clip.micro_fast_head_shake" })
        let microYaw = microShake.poseKeyframes.map { abs($0.yaw) }.max() ?? 0
        XCTAssertLessThanOrEqual(microYaw, 90)

        let normalShake = try XCTUnwrap(clips.first { $0.id == "clip.normal_head_shake" })
        let normalYaw = normalShake.poseKeyframes.map { abs($0.yaw) }.max() ?? 0
        XCTAssertGreaterThanOrEqual(normalYaw, 300)

        let chatSoftIdle = try XCTUnwrap(clips.first { $0.id == "clip.chat_soft_idle" })
        let chatYaw = chatSoftIdle.poseKeyframes.map { abs($0.yaw) }.max() ?? 0
        let chatPitchDelta = chatSoftIdle.poseKeyframes.map { abs($0.pitch - 120) }.max() ?? 0
        XCTAssertTrue(chatSoftIdle.faceEvents.isEmpty)
        XCTAssertLessThanOrEqual(chatYaw, 28)
        XCTAssertLessThanOrEqual(chatPitchDelta, 16)
        XCTAssertGreaterThan(chatSoftIdle.durationMs, 1_800)
    }

    func testTimelineComposerCombinesClipsAndSoundsWithinFifteenSeconds() throws {
        let timeline = ActionTimeline(
            title: "Tiny Combo",
            clips: [
                ActionTimelineClip(clipId: "clip.micro_fast_head_shake", startTimeMs: 0, speedScale: 2.0),
                ActionTimelineClip(clipId: "clip.normal_vertical_nod", startTimeMs: 900, speedScale: 1.0)
            ],
            sounds: [
                SoundEvent(timeMs: 0, assetId: "sound.happy"),
                SoundEvent(timeMs: 900, assetId: "sound.celebrate")
            ]
        )

        let action = try ActionTimelineComposer.compose(timeline, id: "action.tiny_combo")

        XCTAssertEqual(action.id, "action.tiny_combo")
        XCTAssertEqual(action.title, "Tiny Combo")
        XCTAssertLessThanOrEqual(action.durationMs, ActionTimeline.maxDurationMs)
        XCTAssertEqual(action.soundEvents.map(\.assetId), ["sound.happy", "sound.celebrate"])

        let firstClipLastPose = try XCTUnwrap(action.poseKeyframes.filter { $0.timeMs < 900 }.last)
        XCTAssertLessThanOrEqual(firstClipLastPose.timeMs, 300)
        XCTAssertGreaterThanOrEqual(firstClipLastPose.speed, 1000)
    }

    func testTimelineComposerRejectsClipsThatRunPastFifteenSeconds() {
        let timeline = ActionTimeline(
            title: "Too Long",
            clips: [
                ActionTimelineClip(clipId: "clip.normal_vertical_nod", startTimeMs: 14_700, speedScale: 0.5)
            ],
            sounds: []
        )

        XCTAssertThrowsError(try ActionTimelineComposer.compose(timeline, id: "action.too_long")) { error in
            XCTAssertEqual(error as? ActionTimelineComposerError, .timelineTooLong)
        }
    }

    func testTimelineComposerRejectsOverlappingActionClips() {
        let timeline = ActionTimeline(
            title: "Overlap",
            clips: [
                ActionTimelineClip(clipId: "clip.rhythmic_vertical_nod", startTimeMs: 0),
                ActionTimelineClip(clipId: "clip.rhythmic_side_nod", startTimeMs: 400)
            ],
            sounds: []
        )

        XCTAssertThrowsError(try ActionTimelineComposer.compose(timeline, id: "action.overlap")) { error in
            XCTAssertEqual(error as? ActionTimelineComposerError, .overlappingClips)
        }
    }

    func testTimelineTrackMetricsMapTimeAndScaledClipDurationToFractions() throws {
        let clip = try XCTUnwrap(BuiltInActionClipLibrary.clips.first { $0.id == "clip.normal_vertical_nod" })
        let placement = ActionTimelineClip(clipId: clip.id, startTimeMs: 3_000, speedScale: 2.0)

        let frame = ActionTimelineTrackMetrics.frame(for: placement, clip: clip)

        XCTAssertEqual(frame.startFraction, 0.2, accuracy: 0.001)
        XCTAssertEqual(frame.widthFraction, 0.033, accuracy: 0.001)
        XCTAssertEqual(frame.durationMs, 500)
    }

    func testTimelineSequencerPlacesClipsEndToStart() throws {
        let placements = [
            ActionTimelineClip(clipId: "clip.micro_fast_head_shake", startTimeMs: 8_000, speedScale: 1.0),
            ActionTimelineClip(clipId: "clip.rhythmic_vertical_nod", startTimeMs: 0, speedScale: 2.0),
            ActionTimelineClip(clipId: "clip.normal_vertical_nod", startTimeMs: 0, speedScale: 1.0)
        ]

        let sequenced = try ActionTimelineSequencer.sequencedClips(placements)

        XCTAssertEqual(sequenced.map(\.clipId), [
            "clip.micro_fast_head_shake",
            "clip.rhythmic_vertical_nod",
            "clip.normal_vertical_nod"
        ])
        XCTAssertEqual(sequenced.map(\.startTimeMs), [0, 480, 1_280])
        XCTAssertEqual(ActionTimelineSequencer.nextStart(after: sequenced), 2_280)
    }

    func testTimelineSequencerRejectsSequencesPastFifteenSeconds() {
        let placements = [
            ActionTimelineClip(clipId: "clip.rhythmic_vertical_nod", startTimeMs: 0, speedScale: 0.25),
            ActionTimelineClip(clipId: "clip.rhythmic_side_nod", startTimeMs: 0, speedScale: 0.25),
            ActionTimelineClip(clipId: "clip.normal_head_shake", startTimeMs: 0, speedScale: 0.25)
        ]

        XCTAssertThrowsError(try ActionTimelineSequencer.sequencedClips(placements)) { error in
            XCTAssertEqual(error as? ActionTimelineComposerError, .timelineTooLong)
        }
    }

    func testChatMotionPlannerUsesSoftIdleClipForNaturalCallStates() throws {
        let listening = try XCTUnwrap(ChatMotionPlanner.cue(
            isStreaming: true,
            stage: "listening",
            style: .natural,
            randomUnit: 0.25
        ))
        let thinking = try XCTUnwrap(ChatMotionPlanner.cue(
            isStreaming: true,
            stage: "processing",
            style: .natural,
            randomUnit: 0.5
        ))
        let speaking = try XCTUnwrap(ChatMotionPlanner.cue(
            isStreaming: true,
            stage: "speaking",
            style: .natural,
            randomUnit: 0.75
        ))
        let settling = try XCTUnwrap(ChatMotionPlanner.cue(
            isStreaming: true,
            stage: "connected",
            style: .natural,
            randomUnit: 0
        ))

        XCTAssertEqual(listening.clipId, "clip.chat_soft_idle")
        XCTAssertEqual(thinking.clipId, "clip.chat_soft_idle")
        XCTAssertEqual(speaking.clipId, "clip.chat_soft_idle")
        XCTAssertEqual(settling.clipId, "clip.chat_soft_idle")
        XCTAssertEqual(listening.state, .listening)
        XCTAssertEqual(thinking.state, .thinking)
        XCTAssertEqual(speaking.state, .speaking)
        XCTAssertEqual(settling.state, .settling)
        XCTAssertGreaterThan(speaking.speedScale, thinking.speedScale)
        XCTAssertGreaterThan(thinking.nextDelayMs, speaking.nextDelayMs)
        XCTAssertNil(ChatMotionPlanner.cue(
            isStreaming: false,
            stage: "speaking",
            style: .natural,
            randomUnit: 0.5
        ))
    }
}
