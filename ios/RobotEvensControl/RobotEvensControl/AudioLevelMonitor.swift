import AVFoundation
import Foundation

final class AudioLevelMonitor {
    enum MonitorError: Error {
        case permissionDenied
        case inputUnavailable
    }

    private let engine = AVAudioEngine()
    private let onLevel: @MainActor @Sendable (Double) -> Void
    private var hasInstalledTap = false

    init(onLevel: @escaping @MainActor @Sendable (Double) -> Void) {
        self.onLevel = onLevel
    }

    @MainActor
    func start() async throws {
        guard await requestMicrophonePermission() else {
            throw MonitorError.permissionDenied
        }

        do {
            let session = AVAudioSession.sharedInstance()
            try session.setCategory(.playAndRecord, mode: .measurement, options: [.mixWithOthers, .defaultToSpeaker])
            try session.setActive(true)
            guard session.isInputAvailable else {
                throw MonitorError.inputUnavailable
            }

            let inputNode = engine.inputNode
            if hasInstalledTap {
                inputNode.removeTap(onBus: 0)
                hasInstalledTap = false
            }

            inputNode.installTap(
                onBus: 0,
                bufferSize: 1_024,
                format: nil,
                block: Self.makeTapBlock(onLevel: onLevel)
            )
            hasInstalledTap = true

            engine.prepare()
            try engine.start()
        } catch {
            stop()
            throw error
        }
    }

    @MainActor
    func stop() {
        if hasInstalledTap {
            engine.inputNode.removeTap(onBus: 0)
            hasInstalledTap = false
        }
        engine.stop()
        try? AVAudioSession.sharedInstance().setActive(false, options: .notifyOthersOnDeactivation)
    }

    @MainActor
    private func requestMicrophonePermission() async -> Bool {
        switch AVAudioApplication.shared.recordPermission {
        case .granted:
            return true
        case .denied:
            return false
        case .undetermined:
            return await withCheckedContinuation { continuation in
                AVAudioApplication.requestRecordPermission { granted in
                    continuation.resume(returning: granted)
                }
            }
        @unknown default:
            return false
        }
    }

    private nonisolated static func makeTapBlock(
        onLevel: @escaping @MainActor @Sendable (Double) -> Void
    ) -> AVAudioNodeTapBlock {
        { buffer, _ in
            let level = Self.normalizedLevel(from: buffer)
            Task { @MainActor in
                onLevel(level)
            }
        }
    }

    private nonisolated static func normalizedLevel(from buffer: AVAudioPCMBuffer) -> Double {
        guard let channels = buffer.floatChannelData else { return 0 }
        let channelCount = Int(buffer.format.channelCount)
        let frameCount = Int(buffer.frameLength)
        guard channelCount > 0, frameCount > 0 else { return 0 }

        var squaredTotal: Float = 0
        for channelIndex in 0..<channelCount {
            let channel = channels[channelIndex]
            for frameIndex in 0..<frameCount {
                let sample = channel[frameIndex]
                squaredTotal += sample * sample
            }
        }

        let meanSquare = squaredTotal / Float(channelCount * frameCount)
        let rms = sqrt(max(meanSquare, 0))
        let decibels = 20 * log10(max(Double(rms), 0.000_001))
        return audioLevelClamp((decibels + 55) / 49, low: 0, high: 1)
    }
}

private func audioLevelClamp<T: Comparable>(_ value: T, low: T, high: T) -> T {
    min(max(value, low), high)
}
