import CoreMotion
import Foundation

final class CoreMotionPoseSource {
    private let motionManager = CMMotionManager()

    var isAvailable: Bool {
        motionManager.isDeviceMotionAvailable
    }

    func start(updateInterval: TimeInterval, _ handler: @escaping @MainActor (PhoneAttitude) -> Void) {
        guard motionManager.isDeviceMotionAvailable else { return }
        if motionManager.isDeviceMotionActive {
            motionManager.stopDeviceMotionUpdates()
        }
        motionManager.deviceMotionUpdateInterval = updateInterval
        motionManager.startDeviceMotionUpdates(to: .main) { motion, _ in
            guard let attitude = motion?.attitude else { return }
            let phoneAttitude = PhoneAttitude(rollRadians: attitude.roll, pitchRadians: attitude.pitch)
            Task { @MainActor in
                handler(phoneAttitude)
            }
        }
    }

    func stop() {
        guard motionManager.isDeviceMotionActive else { return }
        motionManager.stopDeviceMotionUpdates()
    }
}
