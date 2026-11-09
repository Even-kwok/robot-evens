import Foundation

public struct MotionStreamPacket: Equatable, Sendable {
    public static let defaultPort: UInt16 = 4210

    public let sequence: UInt16
    public let pose: ServoPose

    public init(sequence: UInt16, pose: ServoPose) {
        self.sequence = sequence
        self.pose = pose
    }

    public func encoded() -> Data {
        var data = Data()
        data.reserveCapacity(12)
        data.append(0x52)
        data.append(0x45)
        data.append(0x01)
        data.append(0x00)
        appendUInt16(sequence, to: &data)
        appendInt16(clamp(pose.yaw, low: -1280, high: 1280), to: &data)
        appendInt16(clamp(pose.pitch, low: 0, high: 900), to: &data)
        appendUInt16(UInt16(clamp(pose.speed, low: 0, high: 1000)), to: &data)
        return data
    }
}

private func appendInt16(_ value: Int, to data: inout Data) {
    appendUInt16(UInt16(bitPattern: Int16(value)), to: &data)
}

private func appendUInt16(_ value: UInt16, to data: inout Data) {
    data.append(UInt8((value >> 8) & 0xFF))
    data.append(UInt8(value & 0xFF))
}

private func clamp<T: Comparable>(_ value: T, low: T, high: T) -> T {
    min(max(value, low), high)
}
