import Foundation

public struct MotionControlTuning: Equatable, Sendable {
    public let motionUpdateHz: Int
    public let liveSendIntervalMs: Int
    public let deadZoneRadians: Double
    public let smoothingAlpha: Double

    public init(
        motionUpdateHz: Int,
        liveSendIntervalMs: Int,
        deadZoneRadians: Double,
        smoothingAlpha: Double
    ) {
        self.motionUpdateHz = motionUpdateHz
        self.liveSendIntervalMs = liveSendIntervalMs
        self.deadZoneRadians = deadZoneRadians
        self.smoothingAlpha = smoothingAlpha
    }

    public static let standard = MotionControlTuning(
        motionUpdateHz: 30,
        liveSendIntervalMs: 25,
        deadZoneRadians: 0.025,
        smoothingAlpha: 0.55
    )

    public static let precision = MotionControlTuning(
        motionUpdateHz: 60,
        liveSendIntervalMs: 16,
        deadZoneRadians: 0.004,
        smoothingAlpha: 0.88
    )
}
