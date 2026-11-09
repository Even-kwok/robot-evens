import Foundation

public struct AudioBeatServoConfiguration: Equatable, Sendable {
    private static let fastBeatIntervalMs = 220
    private static let slowBeatIntervalMs = 800

    public var centerYaw: Int
    public var centerPitch: Int
    public var pitchAmplitude: Int
    public var yawAmplitude: Int
    public var isPitchEnabled: Bool
    public var isYawEnabled: Bool
    public var isPitchAmplitudeRandomized: Bool
    public var isYawAmplitudeRandomized: Bool
    public var motionCurve: RhythmMotionCurve
    public var speed: Int
    public var isAdaptiveSpeedEnabled: Bool
    public var adaptiveSlowSpeed: Int
    public var adaptiveFastSpeed: Int
    public var sensitivity: Double
    public var releaseMs: Int
    public var minimumBeatSpacingMs: Int

    public init(
        centerYaw: Int = 0,
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
    ) {
        self.centerYaw = audioBeatClamp(centerYaw, low: -1280, high: 1280)
        self.centerPitch = audioBeatClamp(centerPitch, low: 0, high: 900)
        self.pitchAmplitude = audioBeatClamp(pitchAmplitude, low: 0, high: 900)
        self.yawAmplitude = audioBeatClamp(yawAmplitude, low: 0, high: 1280)
        self.isPitchEnabled = isPitchEnabled
        self.isYawEnabled = isYawEnabled
        self.isPitchAmplitudeRandomized = isPitchAmplitudeRandomized
        self.isYawAmplitudeRandomized = isYawAmplitudeRandomized
        self.motionCurve = motionCurve
        self.speed = audioBeatClamp(speed, low: 0, high: 1000)
        self.isAdaptiveSpeedEnabled = isAdaptiveSpeedEnabled
        let clampedSlowSpeed = audioBeatClamp(adaptiveSlowSpeed, low: 0, high: 1000)
        let clampedFastSpeed = audioBeatClamp(adaptiveFastSpeed, low: 0, high: 1000)
        self.adaptiveSlowSpeed = min(clampedSlowSpeed, clampedFastSpeed)
        self.adaptiveFastSpeed = max(clampedSlowSpeed, clampedFastSpeed)
        self.sensitivity = audioBeatClamp(sensitivity, low: 0, high: 1)
        self.releaseMs = max(40, releaseMs)
        self.minimumBeatSpacingMs = max(40, minimumBeatSpacingMs)
    }

    public var triggerThreshold: Double {
        audioBeatClamp(0.78 - sensitivity * 0.48, low: 0.18, high: 0.78)
    }

    public func adaptiveSpeed(forBeatIntervalMs intervalMs: Int?) -> Int {
        guard isAdaptiveSpeedEnabled, let intervalMs else { return speed }
        let clampedInterval = audioBeatClamp(
            intervalMs,
            low: Self.fastBeatIntervalMs,
            high: Self.slowBeatIntervalMs
        )
        let fastness = Double(Self.slowBeatIntervalMs - clampedInterval) /
            Double(Self.slowBeatIntervalMs - Self.fastBeatIntervalMs)
        return adaptiveSlowSpeed + Int((Double(adaptiveFastSpeed - adaptiveSlowSpeed) * fastness).rounded())
    }

    public func randomizedAmplitude(base: Int, normalizedLevel: Double, randomUnit: Double, isEnabled: Bool) -> Int {
        guard isEnabled, base > 0 else { return base }
        let levelFactor = 0.75 + audioBeatClamp(normalizedLevel, low: 0, high: 1) * 0.25
        let randomFactor = 0.45 + audioBeatClamp(randomUnit, low: 0, high: 1) * 0.55
        return audioBeatClamp(Int((Double(base) * levelFactor * randomFactor).rounded()), low: 1, high: base)
    }
}

public struct AudioBeatServoState: Equatable, Sendable {
    public let pose: ServoPose
    public let normalizedLevel: Double
    public let pulse: Double
    public let isBeat: Bool

    public init(pose: ServoPose, normalizedLevel: Double, pulse: Double, isBeat: Bool) {
        self.pose = pose
        self.normalizedLevel = normalizedLevel
        self.pulse = pulse
        self.isBeat = isBeat
    }
}

public struct AudioBeatServoMapper: Sendable {
    public private(set) var configuration: AudioBeatServoConfiguration
    private var lastBeatAtMs: Int?
    private var activeBeatStartedAtMs: Int?
    private var activePoseSpeed: Int?
    private var activePitchAmplitude: Int?
    private var activeYawAmplitude: Int?
    private var yawDirection = 1

    public init(configuration: AudioBeatServoConfiguration = AudioBeatServoConfiguration()) {
        self.configuration = configuration
    }

    public mutating func update(configuration: AudioBeatServoConfiguration) {
        self.configuration = configuration
        activePitchAmplitude = nil
        activeYawAmplitude = nil
    }

    public mutating func reset() {
        lastBeatAtMs = nil
        activeBeatStartedAtMs = nil
        activePoseSpeed = nil
        activePitchAmplitude = nil
        activeYawAmplitude = nil
        yawDirection = 1
    }

    public mutating func map(level: Double, atMs: Int, randomUnit: Double? = nil) -> AudioBeatServoState {
        let normalizedLevel = audioBeatClamp(level, low: 0, high: 1)
        let now = max(0, atMs)
        let canTrigger = lastBeatAtMs.map { now - $0 >= configuration.minimumBeatSpacingMs } ?? true
        let isBeat = normalizedLevel >= configuration.triggerThreshold && canTrigger

        if isBeat {
            let beatIntervalMs = lastBeatAtMs.map { now - $0 }
            lastBeatAtMs = now
            activeBeatStartedAtMs = now
            activePoseSpeed = configuration.adaptiveSpeed(forBeatIntervalMs: beatIntervalMs)
            let pitchRandomUnit = randomUnit ?? Double.random(in: 0...1)
            let yawRandomUnit = randomUnit ?? Double.random(in: 0...1)
            activePitchAmplitude = configuration.randomizedAmplitude(
                base: configuration.pitchAmplitude,
                normalizedLevel: normalizedLevel,
                randomUnit: pitchRandomUnit,
                isEnabled: configuration.isPitchAmplitudeRandomized
            )
            activeYawAmplitude = configuration.randomizedAmplitude(
                base: configuration.yawAmplitude,
                normalizedLevel: normalizedLevel,
                randomUnit: yawRandomUnit,
                isEnabled: configuration.isYawAmplitudeRandomized
            )
            yawDirection *= -1
        }

        let pulse = currentPulse(atMs: now)
        if pulse <= 0 {
            activeBeatStartedAtMs = nil
            activePoseSpeed = nil
            activePitchAmplitude = nil
            activeYawAmplitude = nil
        }
        let poseSpeed = configuration.isAdaptiveSpeedEnabled && pulse > 0
            ? (activePoseSpeed ?? configuration.speed)
            : configuration.speed

        let pitchOffset = configuration.isPitchEnabled
            ? Int((Double(activePitchAmplitude ?? configuration.pitchAmplitude) * pulse).rounded())
            : 0
        let yawOffset = configuration.isYawEnabled
            ? Int((Double(activeYawAmplitude ?? configuration.yawAmplitude) * pulse).rounded()) * -yawDirection
            : 0
        let pose = ServoPose(
            yaw: audioBeatClamp(configuration.centerYaw + yawOffset, low: -1280, high: 1280),
            pitch: audioBeatClamp(configuration.centerPitch + pitchOffset, low: 0, high: 900),
            speed: poseSpeed
        )
        return AudioBeatServoState(
            pose: pose,
            normalizedLevel: normalizedLevel,
            pulse: pulse,
            isBeat: isBeat
        )
    }

    private func currentPulse(atMs now: Int) -> Double {
        guard let activeBeatStartedAtMs else { return 0 }
        let elapsedMs = max(0, now - activeBeatStartedAtMs)
        guard elapsedMs <= configuration.releaseMs else { return 0 }
        return configuration.motionCurve.releasePulse(at: Double(elapsedMs) / Double(configuration.releaseMs))
    }
}

private func audioBeatClamp<T: Comparable>(_ value: T, low: T, high: T) -> T {
    min(max(value, low), high)
}
