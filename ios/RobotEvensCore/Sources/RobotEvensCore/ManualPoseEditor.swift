import Foundation

public struct ManualPoseEditor: Sendable {
    public private(set) var pose: ServoPose

    public init(pose: ServoPose = ServoPose(yaw: 0, pitch: 120, speed: 700)) {
        self.pose = ServoPose(
            yaw: clamp(pose.yaw, low: -1280, high: 1280),
            pitch: clamp(pose.pitch, low: 0, high: 900),
            speed: clamp(pose.speed, low: 0, high: 1000)
        )
    }

    public mutating func setYaw(_ yaw: Int) {
        pose = ServoPose(yaw: clamp(yaw, low: -1280, high: 1280), pitch: pose.pitch, speed: pose.speed)
    }

    public mutating func setPitch(_ pitch: Int) {
        pose = ServoPose(yaw: pose.yaw, pitch: clamp(pitch, low: 0, high: 900), speed: pose.speed)
    }

    public mutating func setSpeed(_ speed: Int) {
        pose = ServoPose(yaw: pose.yaw, pitch: pose.pitch, speed: clamp(speed, low: 0, high: 1000))
    }
}

private func clamp<T: Comparable>(_ value: T, low: T, high: T) -> T {
    min(max(value, low), high)
}
