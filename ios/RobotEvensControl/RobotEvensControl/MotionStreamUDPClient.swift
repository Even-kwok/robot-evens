import Foundation
import Network

final class MotionStreamUDPClient {
    private let queue = DispatchQueue(label: "robot-evens.motion-stream")
    private var connection: NWConnection?
    private var sequence: UInt16 = 0

    func configure(baseURL: URL, port: UInt16 = MotionStreamPacket.defaultPort) {
        guard let host = baseURL.host(), let endpointPort = NWEndpoint.Port(rawValue: port) else {
            close()
            return
        }

        close()
        sequence = 0
        let endpointHost = NWEndpoint.Host(host)
        let connection = NWConnection(host: endpointHost, port: endpointPort, using: .udp)
        self.connection = connection
        connection.start(queue: queue)
    }

    @discardableResult
    func send(_ pose: ServoPose) -> Bool {
        guard let connection else { return false }
        let packet = MotionStreamPacket(sequence: sequence, pose: pose)
        sequence &+= 1
        connection.send(content: packet.encoded(), completion: .contentProcessed { _ in })
        return true
    }

    func close() {
        connection?.cancel()
        connection = nil
    }
}
