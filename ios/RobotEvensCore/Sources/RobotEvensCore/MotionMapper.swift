import Foundation

public struct PhoneAttitude: Equatable, Sendable {
    public static let zero = PhoneAttitude(rollRadians: 0, pitchRadians: 0)

    public let rollRadians: Double
    public let pitchRadians: Double

    public init(rollRadians: Double, pitchRadians: Double) {
        self.rollRadians = rollRadians
        self.pitchRadians = pitchRadians
    }
}

public struct ServoPose: Equatable, Sendable {
    public let yaw: Int
    public let pitch: Int
    public let speed: Int

    public init(yaw: Int, pitch: Int, speed: Int) {
        self.yaw = yaw
        self.pitch = pitch
        self.speed = speed
    }
}

public struct MotionMapperConfiguration: Equatable, Sendable {
    public let maxRollRadians: Double
    public let maxPitchRadians: Double
    public let deadZoneRadians: Double
    public let smoothingAlpha: Double
    public let restPitch: Int
    public let speed: Int

    public init(
        maxRollRadians: Double = .pi / 4,
        maxPitchRadians: Double = .pi / 4,
        deadZoneRadians: Double = 0.025,
        smoothingAlpha: Double = 0.35,
        restPitch: Int = 120,
        speed: Int = 260
    ) {
        self.maxRollRadians = maxRollRadians
        self.maxPitchRadians = maxPitchRadians
        self.deadZoneRadians = deadZoneRadians
        self.smoothingAlpha = smoothingAlpha
        self.restPitch = restPitch
        self.speed = speed
    }
}

public struct MotionMapper: Sendable {
    private let configuration: MotionMapperConfiguration
    private var neutralAttitude: PhoneAttitude = .zero
    private var commandedSpeed: Int
    private var lastPose: ServoPose

    public init(configuration: MotionMapperConfiguration = MotionMapperConfiguration()) {
        self.configuration = configuration
        self.commandedSpeed = clamp(configuration.speed, low: 0, high: 1000)
        self.lastPose = ServoPose(yaw: 0, pitch: configuration.restPitch, speed: commandedSpeed)
    }

    public mutating func calibrate(_ attitude: PhoneAttitude) {
        neutralAttitude = attitude
        lastPose = ServoPose(yaw: 0, pitch: configuration.restPitch, speed: commandedSpeed)
    }

    public mutating func setSpeed(_ speed: Int) {
        commandedSpeed = clamp(speed, low: 0, high: 1000)
        lastPose = ServoPose(yaw: lastPose.yaw, pitch: lastPose.pitch, speed: commandedSpeed)
    }

    public mutating func map(_ attitude: PhoneAttitude) -> ServoPose {
        let relativeRoll = applyDeadZone(attitude.rollRadians - neutralAttitude.rollRadians)
        let relativePitch = applyDeadZone(attitude.pitchRadians - neutralAttitude.pitchRadians)

        let yawTarget = clamp(
            Int((relativeRoll / configuration.maxRollRadians * 1280.0).rounded()),
            low: -1280,
            high: 1280
        )
        let pitchRangeUp = 900 - configuration.restPitch
        let pitchTarget = clamp(
            configuration.restPitch + Int((relativePitch / configuration.maxPitchRadians * Double(pitchRangeUp)).rounded()),
            low: 0,
            high: 900
        )

        let alpha = clamp(configuration.smoothingAlpha, low: 0.0, high: 1.0)
        let pose = ServoPose(
            yaw: smooth(previous: lastPose.yaw, target: yawTarget, alpha: alpha),
            pitch: smooth(previous: lastPose.pitch, target: pitchTarget, alpha: alpha),
            speed: commandedSpeed
        )
        lastPose = pose
        return pose
    }

    private func applyDeadZone(_ value: Double) -> Double {
        abs(value) < configuration.deadZoneRadians ? 0 : value
    }

    private func smooth(previous: Int, target: Int, alpha: Double) -> Int {
        Int((Double(previous) + (Double(target - previous) * alpha)).rounded())
    }
}

private func clamp<T: Comparable>(_ value: T, low: T, high: T) -> T {
    min(max(value, low), high)
}
