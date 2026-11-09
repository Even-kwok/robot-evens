import Foundation

public struct PlaybackCommand: Equatable, Sendable {
    public let timeMs: Int
    public let request: RobotRequest

    public init(timeMs: Int, request: RobotRequest) {
        self.timeMs = timeMs
        self.request = request
    }
}

public enum PlaybackScheduler {
    public static func commands(for action: RobotAction) -> [PlaybackCommand] {
        var scheduled: [(priority: Int, command: PlaybackCommand)] = []

        if let sound = action.startSoundAssetId, !sound.isEmpty {
            scheduled.append((0, PlaybackCommand(timeMs: 0, request: RobotEndpointBuilder.invokeAsset(sound))))
        }

        for sound in action.soundEvents where !sound.assetId.isEmpty {
            scheduled.append((0, PlaybackCommand(timeMs: sound.timeMs, request: RobotEndpointBuilder.invokeAsset(sound.assetId))))
        }

        for pose in action.poseKeyframes {
            let servoPose = ServoPose(yaw: pose.yaw, pitch: pose.pitch, speed: pose.speed)
            scheduled.append((1, PlaybackCommand(timeMs: pose.timeMs, request: RobotEndpointBuilder.pose(servoPose))))
        }

        for face in action.faceEvents {
            scheduled.append((2, PlaybackCommand(timeMs: face.timeMs, request: RobotEndpointBuilder.face(mood: face.mood))))
        }

        if action.endMode == .home {
            scheduled.append((3, PlaybackCommand(timeMs: action.durationMs, request: RobotEndpointBuilder.home())))
        }

        return scheduled
            .sorted { left, right in
                if left.command.timeMs == right.command.timeMs {
                    return left.priority < right.priority
                }
                return left.command.timeMs < right.command.timeMs
            }
            .map(\.command)
    }
}
