import Foundation
import zlib

public enum JSONValue: Equatable, Sendable {
    case string(String)
    case int(Int)
    case double(Double)
    case bool(Bool)
    case array([JSONValue])
    case object([String: JSONValue])

    var objectValue: Any {
        switch self {
        case .string(let value):
            value
        case .int(let value):
            value
        case .double(let value):
            value
        case .bool(let value):
            value
        case .array(let values):
            values.map(\.objectValue)
        case .object(let values):
            values.mapValues(\.objectValue)
        }
    }
}

public struct RobotRequest: Equatable, Sendable {
    public let method: String
    public let path: String
    public let body: [String: JSONValue]

    public init(method: String, path: String, body: [String: JSONValue] = [:]) {
        self.method = method
        self.path = path
        self.body = body
    }

    public func urlRequest(baseURL: URL) throws -> URLRequest {
        let url = baseURL.appending(path: path)
        var request = URLRequest(url: url)
        request.httpMethod = method
        if !body.isEmpty {
            let object = body.mapValues(\.objectValue)
            request.httpBody = try JSONSerialization.data(withJSONObject: object, options: [.sortedKeys])
            request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        }
        return request
    }
}

public struct RobotMicSample: Decodable, Equatable, Sendable {
    public let ok: Bool
    public let micEnabled: Bool
    public let recordAccepted: Bool
    public let level: Double

    public init(ok: Bool = false, micEnabled: Bool = false, recordAccepted: Bool = false, level: Double = 0) {
        self.ok = ok
        self.micEnabled = micEnabled
        self.recordAccepted = recordAccepted
        self.level = clamp(level, low: 0, high: 1)
    }

    private enum CodingKeys: String, CodingKey {
        case ok
        case micEnabled
        case recordAccepted
        case level
    }

    public init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        self.init(
            ok: try container.decodeIfPresent(Bool.self, forKey: .ok) ?? false,
            micEnabled: try container.decodeIfPresent(Bool.self, forKey: .micEnabled) ?? false,
            recordAccepted: try container.decodeIfPresent(Bool.self, forKey: .recordAccepted) ?? false,
            level: try container.decodeIfPresent(Double.self, forKey: .level) ?? 0
        )
    }
}

public enum RobotEndpointBuilder {
    public static func ping() -> RobotRequest {
        RobotRequest(method: "GET", path: "/api/ping")
    }

    public static func pose(_ pose: ServoPose) -> RobotRequest {
        return RobotRequest(
            method: "POST",
            path: "/api/motion/pose",
            body: [
                "yaw": .int(clamp(pose.yaw, low: -1280, high: 1280)),
                "pitch": .int(clamp(pose.pitch, low: 0, high: 900)),
                "speed": .int(clamp(pose.speed, low: 0, high: 1000))
            ]
        )
    }

    public static func face(mood: String, color: String? = nil) -> RobotRequest {
        var body: [String: JSONValue] = ["mood": .string(mood)]
        if let color, !color.isEmpty {
            body["color"] = .string(color)
        }
        return RobotRequest(method: "POST", path: "/api/face/set", body: body)
    }

    public static func faceStyle(_ style: RobotFaceStyle, mood: String? = nil) -> RobotRequest {
        var body = faceStyleObject(style)
        if let mood, !mood.isEmpty { body["mood"] = .string(mood) }
        return RobotRequest(method: "POST", path: "/api/face/style", body: body)
    }

    public static func eyeLibrary() -> RobotRequest {
        RobotRequest(method: "GET", path: "/api/face/library")
    }

    public static func saveSystemEyeLibraryEntry(
        id: String,
        mood: String,
        style: RobotFaceStyle,
        expectedRevision: Int
    ) -> RobotRequest {
        RobotRequest(
            method: "POST",
            path: "/api/face/library/save",
            body: [
                "kind": .string("system"),
                "id": .string(id),
                "baseMood": .string(mood),
                "expectedRevision": .int(expectedRevision),
                "style": .object(faceStyleObject(style))
            ]
        )
    }

    public static func saveCustomEyeLibraryEntry(
        id: String?,
        design: EyeShapeDesign,
        expectedRevision: Int
    ) -> RobotRequest {
        var body: [String: JSONValue] = [
            "kind": .string(design.isGlyph ? "glyph" : "custom"),
            "name": .string(design.name),
            "baseMood": .string(design.baseMood),
            "sweepEnabled": .bool(design.sweepEnabled),
            "mirrorRightEye": .bool(design.mirrorRightEye),
            "smoothingEnabled": .bool(design.smoothingEnabled),
            "expectedRevision": .int(expectedRevision),
            "style": .object(faceStyleObject(design.style))
        ]
        if let glyphId = design.glyphId {
            body["glyphId"] = .string(glyphId)
        } else {
            body["points"] = .array(customEyePoints(design))
        }
        if let id, !id.isEmpty { body["id"] = .string(id) }
        return RobotRequest(method: "POST", path: "/api/face/library/save", body: body)
    }

    public static func deleteEyeLibraryEntry(id: String, expectedRevision: Int) -> RobotRequest {
        RobotRequest(
            method: "POST",
            path: "/api/face/library/delete",
            body: ["id": .string(id), "expectedRevision": .int(expectedRevision)]
        )
    }

    public static func activateEyeLibraryEntry(id: String, expectedRevision: Int) -> RobotRequest {
        RobotRequest(
            method: "POST",
            path: "/api/face/library/activate",
            body: ["id": .string(id), "expectedRevision": .int(expectedRevision)]
        )
    }

    public static func customEyeShape(_ design: EyeShapeDesign, enabled: Bool = true) -> RobotRequest {
        var body: [String: JSONValue] = [
            "enabled": .bool(enabled),
            "name": .string(design.name),
            "sweepEnabled": .bool(design.sweepEnabled),
            "mirrorRightEye": .bool(design.mirrorRightEye),
            "smoothingEnabled": .bool(design.smoothingEnabled)
        ]
        if let glyphId = design.glyphId {
            body["glyphId"] = .string(glyphId)
        } else {
            body["points"] = .array(customEyePoints(design))
        }
        return RobotRequest(
            method: "POST",
            path: "/api/face/shape",
            body: body
        )
    }

    public static func customEyePreview(
        _ design: EyeShapeDesign,
        durationMs: Int = 10_000,
        blinkEnabled: Bool = false,
        blinkNow: Bool = false
    ) -> RobotRequest {
        let style = design.style
        var body: [String: JSONValue] = [
            "name": .string(design.name),
            "sweepEnabled": .bool(design.sweepEnabled),
            "mirrorRightEye": .bool(design.mirrorRightEye),
            "smoothingEnabled": .bool(design.smoothingEnabled),
            "durationMs": .int(clamp(durationMs, low: 1_000, high: 15_000)),
            "blinkEnabled": .bool(blinkEnabled),
            "blinkNow": .bool(blinkNow),
            "eyeWidthScale": .double(style.eyeWidthScale),
            "eyeHeightScale": .double(style.eyeHeightScale),
            "cornerRadius": .int(style.cornerRadius),
            "eyeOffsetX": .int(style.eyeOffsetX),
            "eyeOffsetY": .int(style.eyeOffsetY),
            "eyeSpacing": .int(style.eyeSpacing),
            "eyeColor": .string(style.eyeColor),
            "glowEnabled": .bool(style.glowEnabled),
            "glowStrength": .int(style.glowStrength),
            "scanlinesEnabled": .bool(style.scanlinesEnabled),
            "scanlineSpacing": .int(style.scanlineSpacing),
            "scanlineAlpha": .int(style.scanlineAlpha)
        ]
        if let glyphId = design.glyphId {
            body["glyphId"] = .string(glyphId)
        } else {
            body["points"] = .array(customEyePoints(design))
        }
        return RobotRequest(
            method: "POST",
            path: "/api/face/preview",
            body: body
        )
    }

    public static func stopCustomEyePreview() -> RobotRequest {
        RobotRequest(method: "POST", path: "/api/face/preview/stop")
    }

    public static func customEyePreviewStatus() -> RobotRequest {
        RobotRequest(method: "GET", path: "/api/face/preview/status")
    }

    private static func customEyePoints(_ design: EyeShapeDesign) -> [JSONValue] {
        design.points.prefix(24).enumerated().map { index, point in
            var payload: [String: JSONValue] = [
                "x": .double(point.x),
                "y": .double(point.y),
                "smooth": .bool(design.isPointSmooth(at: index))
            ]
            if let handles = design.effectiveCurveHandles(at: index) {
                payload["handleInX"] = .double(handles.incoming.x)
                payload["handleInY"] = .double(handles.incoming.y)
                payload["handleOutX"] = .double(handles.outgoing.x)
                payload["handleOutY"] = .double(handles.outgoing.y)
            }
            return .object(payload)
        }
    }

    private static func faceStyleObject(_ style: RobotFaceStyle) -> [String: JSONValue] {
        [
            "eyeWidthScale": .double(style.eyeWidthScale),
            "eyeHeightScale": .double(style.eyeHeightScale),
            "cornerRadius": .int(style.cornerRadius),
            "eyeOffsetX": .int(style.eyeOffsetX),
            "eyeOffsetY": .int(style.eyeOffsetY),
            "eyeSpacing": .int(style.eyeSpacing),
            "eyeColor": .string(style.eyeColor),
            "glowEnabled": .bool(style.glowEnabled),
            "glowStrength": .int(style.glowStrength),
            "scanlinesEnabled": .bool(style.scanlinesEnabled),
            "scanlineSpacing": .int(style.scanlineSpacing),
            "scanlineAlpha": .int(style.scanlineAlpha)
        ]
    }

    public static func rgb(
        color: String? = nil,
        side: String? = nil,
        index: Int? = nil,
        brightness: Int? = nil,
        breathing: Bool? = nil,
        marquee: Bool? = nil,
        rainbow: Bool? = nil,
        preset: String? = nil,
        temporary: Bool? = nil
    ) -> RobotRequest {
        var body: [String: JSONValue] = [:]
        if let color, !color.isEmpty {
            body["color"] = .string(color)
        }
        if let side, !side.isEmpty {
            body["side"] = .string(side)
        }
        if let index {
            body["index"] = .int(clamp(index, low: 0, high: 11))
        }
        if let brightness {
            body["brightness"] = .int(clamp(brightness, low: 0, high: 255))
        }
        if let breathing {
            body["breathing"] = .bool(breathing)
        }
        if let marquee {
            body["marquee"] = .bool(marquee)
        }
        if let rainbow {
            body["rainbow"] = .bool(rainbow)
        }
        if let preset, !preset.isEmpty {
            body["preset"] = .string(preset)
        }
        if let temporary {
            body["temporary"] = .bool(temporary)
        }
        return RobotRequest(method: "POST", path: "/api/rgb", body: body)
    }

    public static func restoreStandbyRGB() -> RobotRequest {
        RobotRequest(method: "POST", path: "/api/rgb/restore")
    }

    public static func invokeAsset(_ assetId: String) -> RobotRequest {
        RobotRequest(
            method: "POST",
            path: "/api/asset/invoke",
            body: ["assetId": .string(assetId)]
        )
    }

    public static func home(speed: Int = 300) -> RobotRequest {
        RobotRequest(
            method: "POST",
            path: "/api/motion/home",
            body: ["speed": .int(clamp(speed, low: 0, high: 1000))]
        )
    }

    public static func status() -> RobotRequest {
        RobotRequest(method: "GET", path: "/api/status")
    }

    public static func motionStreamStatus() -> RobotRequest {
        RobotRequest(method: "GET", path: "/api/motion/stream/status")
    }

    public static func micSample(samples: Int = 128, sampleRate: Int = 16_000) -> RobotRequest {
        RobotRequest(
            method: "POST",
            path: "/api/mic/sample",
            body: [
                "samples": .int(clamp(samples, low: 64, high: 512)),
                "sampleRate": .int(clamp(sampleRate, low: 8_000, high: 24_000))
            ]
        )
    }

    public static func catalogAssets() -> RobotRequest {
        RobotRequest(method: "GET", path: "/api/catalog/assets")
    }

    public static func mumbleSet(enabled: Bool) -> RobotRequest {
        RobotRequest(
            method: "POST",
            path: "/api/mumble/set",
            body: ["enabled": .bool(enabled)]
        )
    }

    public static func mumblePlay(mode: String = "full") -> RobotRequest {
        RobotRequest(
            method: "POST",
            path: "/api/mumble/play",
            body: ["mode": .string(mode)]
        )
    }

    public static func emotionSoundSlotSet(slot: EmotionSoundSlot, enabled: Bool = true) -> RobotRequest {
        RobotRequest(
            method: "POST",
            path: "/api/emotion/sound-slot/set",
            body: ["slot": .string(slot.rawValue), "enabled": .bool(enabled)]
        )
    }

    public static func emotionSoundSlotPlay(slot: EmotionSoundSlot, durationMs: Int = 0) -> RobotRequest {
        RobotRequest(
            method: "POST",
            path: "/api/emotion/sound-slot/play",
            body: [
                "slot": .string(slot.rawValue),
                "durationMs": .int(min(max(durationMs, 0), 20_000))
            ]
        )
    }
}

public struct RobotVoicePlaybackResult: Equatable, Sendable {
    public let ok: Bool
    public let sourceSampleRate: Int
    public let sampleRate: Int
    public let sourceBytes: Int
    public let uploadedBytes: Int
    public let error: String?

    public init(
        ok: Bool,
        sourceSampleRate: Int,
        sampleRate: Int,
        sourceBytes: Int,
        uploadedBytes: Int,
        error: String? = nil
    ) {
        self.ok = ok
        self.sourceSampleRate = sourceSampleRate
        self.sampleRate = sampleRate
        self.sourceBytes = sourceBytes
        self.uploadedBytes = uploadedBytes
        self.error = error
    }
}

public protocol RobotHTTPTransport: Sendable {
    func send(_ request: RobotRequest, baseURL: URL) async throws -> Data
    func upload(
        path: String,
        fieldName: String,
        filename: String,
        data: Data,
        contentType: String,
        baseURL: URL
    ) async throws -> Data
}

public struct URLSessionRobotHTTPTransport: RobotHTTPTransport, Sendable {
    public init() {}

    public func send(_ request: RobotRequest, baseURL: URL) async throws -> Data {
        let urlRequest = try request.urlRequest(baseURL: baseURL)
        let (data, _) = try await URLSession.shared.data(for: urlRequest)
        return data
    }

    public func upload(
        path: String,
        fieldName: String,
        filename: String,
        data: Data,
        contentType: String,
        baseURL: URL
    ) async throws -> Data {
        let boundary = "RobotEvens-\(UUID().uuidString)"
        let uploadURL = URL(string: path, relativeTo: baseURL)?.absoluteURL ?? baseURL.appending(path: path)
        var request = URLRequest(url: uploadURL)
        request.httpMethod = "POST"
        request.setValue("multipart/form-data; boundary=\(boundary)", forHTTPHeaderField: "Content-Type")

        var body = Data()
        body.appendUTF8("--\(boundary)\r\n")
        body.appendUTF8("Content-Disposition: form-data; name=\"\(fieldName)\"; filename=\"\(filename)\"\r\n")
        body.appendUTF8("Content-Type: \(contentType)\r\n\r\n")
        body.append(data)
        body.appendUTF8("\r\n--\(boundary)--\r\n")
        request.httpBody = body

        let (responseData, _) = try await URLSession.shared.data(for: request)
        return responseData
    }
}

public struct RobotClient: Sendable {
    private let baseURL: URL
    private let transport: any RobotHTTPTransport

    public init(baseURL: URL, transport: RobotHTTPTransport = URLSessionRobotHTTPTransport()) {
        self.baseURL = baseURL
        self.transport = transport
    }

    @discardableResult
    public func send(_ request: RobotRequest) async throws -> Data {
        try await transport.send(request, baseURL: baseURL)
    }

    public func ping() async throws -> Data {
        try await send(RobotEndpointBuilder.ping())
    }

    public func status() async throws -> Data {
        try await send(RobotEndpointBuilder.status())
    }

    public func motionStreamStatus() async throws -> Data {
        try await send(RobotEndpointBuilder.motionStreamStatus())
    }

    public func micSample(samples: Int = 128, sampleRate: Int = 16_000) async throws -> RobotMicSample {
        let data = try await send(RobotEndpointBuilder.micSample(samples: samples, sampleRate: sampleRate))
        return try JSONDecoder().decode(RobotMicSample.self, from: data)
    }

    public func setPose(_ pose: ServoPose) async throws {
        try await send(RobotEndpointBuilder.pose(pose))
    }

    public func setFace(mood: String, color: String? = nil) async throws {
        try await send(RobotEndpointBuilder.face(mood: mood, color: color))
    }

    public func applyFaceStyle(_ style: RobotFaceStyle, mood: String? = nil) async throws -> RobotMutationResult {
        RobotMutationResult(data: try await send(RobotEndpointBuilder.faceStyle(style, mood: mood)))
    }

    public func eyeLibrary() async throws -> RobotEyeLibrarySnapshot {
        let data = try await send(RobotEndpointBuilder.eyeLibrary())
        return try JSONDecoder().decode(RobotEyeLibrarySnapshot.self, from: data)
    }

    public func saveSystemEyeLibraryEntry(
        id: String,
        mood: String,
        style: RobotFaceStyle,
        expectedRevision: Int
    ) async throws -> RobotEyeLibrarySnapshot {
        let data = try await send(
            RobotEndpointBuilder.saveSystemEyeLibraryEntry(
                id: id,
                mood: mood,
                style: style,
                expectedRevision: expectedRevision
            )
        )
        return try JSONDecoder().decode(RobotEyeLibrarySnapshot.self, from: data)
    }

    public func saveCustomEyeLibraryEntry(
        id: String?,
        design: EyeShapeDesign,
        expectedRevision: Int
    ) async throws -> RobotEyeLibrarySnapshot {
        let data = try await send(
            RobotEndpointBuilder.saveCustomEyeLibraryEntry(
                id: id,
                design: design,
                expectedRevision: expectedRevision
            )
        )
        return try JSONDecoder().decode(RobotEyeLibrarySnapshot.self, from: data)
    }

    public func deleteEyeLibraryEntry(id: String, expectedRevision: Int) async throws -> RobotEyeLibrarySnapshot {
        let data = try await send(RobotEndpointBuilder.deleteEyeLibraryEntry(id: id, expectedRevision: expectedRevision))
        return try JSONDecoder().decode(RobotEyeLibrarySnapshot.self, from: data)
    }

    public func activateEyeLibraryEntry(id: String, expectedRevision: Int) async throws -> RobotEyeLibrarySnapshot {
        let data = try await send(RobotEndpointBuilder.activateEyeLibraryEntry(id: id, expectedRevision: expectedRevision))
        return try JSONDecoder().decode(RobotEyeLibrarySnapshot.self, from: data)
    }

    public func applyEyeShapeDesign(_ design: EyeShapeDesign) async throws -> RobotMutationResult {
        let styleResult = try await applyFaceStyle(design.style, mood: design.baseMood)
        guard styleResult.ok else { return styleResult }
        return RobotMutationResult(data: try await send(RobotEndpointBuilder.customEyeShape(design)))
    }

    public func clearCustomEyeShape() async throws -> RobotMutationResult {
        let design = EyeShapeDesign(points: [])
        return RobotMutationResult(data: try await send(RobotEndpointBuilder.customEyeShape(design, enabled: false)))
    }

    public func previewEyeShapeDesign(
        _ design: EyeShapeDesign,
        durationMs: Int = 10_000,
        blinkEnabled: Bool = false,
        blinkNow: Bool = false
    ) async throws -> RobotMutationResult {
        RobotMutationResult(
            data: try await send(
                RobotEndpointBuilder.customEyePreview(
                    design,
                    durationMs: durationMs,
                    blinkEnabled: blinkEnabled,
                    blinkNow: blinkNow
                )
            )
        )
    }

    public func stopEyeShapePreview() async throws -> RobotMutationResult {
        RobotMutationResult(data: try await send(RobotEndpointBuilder.stopCustomEyePreview()))
    }

    public func setRGB(
        color: String? = nil,
        side: String? = nil,
        index: Int? = nil,
        brightness: Int? = nil,
        breathing: Bool? = nil,
        marquee: Bool? = nil,
        rainbow: Bool? = nil,
        preset: String? = nil,
        temporary: Bool? = nil
    ) async throws {
        try await send(RobotEndpointBuilder.rgb(
            color: color,
            side: side,
            index: index,
            brightness: brightness,
            breathing: breathing,
            marquee: marquee,
            rainbow: rainbow,
            preset: preset,
            temporary: temporary
        ))
    }

    public func restoreStandbyRGB() async throws {
        try await send(RobotEndpointBuilder.restoreStandbyRGB())
    }

    public func invokeAsset(_ assetId: String) async throws {
        try await send(RobotEndpointBuilder.invokeAsset(assetId))
    }

    public func home(speed: Int = 300) async throws {
        try await send(RobotEndpointBuilder.home(speed: speed))
    }

    public func uploadEmotionSoundSlot(
        _ slot: EmotionSoundSlot,
        rawU8Audio: Data,
        playAfterUpload: Bool = true
    ) async throws -> RobotMutationResult {
        let encodedSlot = slot.rawValue.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed) ?? slot.rawValue
        let uploadData = try await transport.upload(
            path: "/api/emotion/sound-slot/upload?slot=\(encodedSlot)",
            fieldName: "file",
            filename: "\(slot.rawValue).raw",
            data: Data(rawU8Audio.prefix(EmotionSoundProcessor.maximumSampleCount)),
            contentType: "application/octet-stream",
            baseURL: baseURL
        )
        let uploadResult = RobotMutationResult(data: uploadData)
        guard uploadResult.ok else { return uploadResult }

        let enabled = RobotMutationResult(
            data: try await transport.send(
                RobotEndpointBuilder.emotionSoundSlotSet(slot: slot, enabled: true),
                baseURL: baseURL
            )
        )
        guard enabled.ok, playAfterUpload else { return enabled }
        return RobotMutationResult(
            data: try await transport.send(
                RobotEndpointBuilder.emotionSoundSlotPlay(slot: slot),
                baseURL: baseURL
            )
        )
    }

    public func playRealtimeVoice(
        _ audioPcm16: Data,
        sampleRate: Int = 24_000,
        audioFormat: String = "pcm_s16le"
    ) async throws -> RobotVoicePlaybackResult {
        let sourceRate = sampleRate > 0 ? sampleRate : 24_000
        let rawAudio = RobotAudioConverter.pcmS16LEToRawU8Mono(
            audioPcm16,
            sourceRate: sourceRate,
            targetRate: 16_000
        )
        _ = try await transport.upload(
            path: "/api/mumble/upload",
            fieldName: "file",
            filename: "realtime-api.raw",
            data: rawAudio,
            contentType: "application/octet-stream",
            baseURL: baseURL
        )
        let enabled = try await transport.send(RobotEndpointBuilder.mumbleSet(enabled: true), baseURL: baseURL)
        let play = try await transport.send(RobotEndpointBuilder.mumblePlay(mode: "full"), baseURL: baseURL)
        let enabledResult = RobotOperationResult(data: enabled)
        let playResult = RobotOperationResult(data: play)
        return RobotVoicePlaybackResult(
            ok: enabledResult.ok && playResult.ok,
            sourceSampleRate: sourceRate,
            sampleRate: 16_000,
            sourceBytes: audioPcm16.count,
            uploadedBytes: rawAudio.count,
            error: enabledResult.error ?? playResult.error
        )
    }
}

public enum RobotAudioConverter {
    public static func pcmS16LEToRawU8Mono(
        _ pcm16: Data,
        sourceRate: Int,
        targetRate: Int = 16_000
    ) -> Data {
        guard sourceRate > 0, targetRate > 0, pcm16.count >= 2 else {
            return Data()
        }
        let sampleCount = pcm16.count / 2
        var samples = [Double]()
        samples.reserveCapacity(sampleCount)
        for index in 0..<sampleCount {
            let low = UInt16(pcm16[index * 2])
            let high = UInt16(pcm16[index * 2 + 1])
            let unsigned = low | (high << 8)
            samples.append(Double(Int16(bitPattern: unsigned)))
        }

        let targetCount = max(1, Int((Double(sampleCount) * Double(targetRate) / Double(sourceRate)).rounded()))
        var output = Data()
        output.reserveCapacity(targetCount)
        for index in 0..<targetCount {
            let sourcePosition = Double(index) * Double(sourceRate) / Double(targetRate)
            let lower = min(max(Int(sourcePosition.rounded(.down)), 0), sampleCount - 1)
            let upper = min(lower + 1, sampleCount - 1)
            let fraction = sourcePosition - Double(lower)
            let sample = samples[lower] + (samples[upper] - samples[lower]) * fraction
            let byte = clamp(Int(((sample + 32_768) / 256).rounded(.down)), low: 0, high: 255)
            output.append(UInt8(byte))
        }
        return output
    }
}

public struct VolcengineRealtimeMobileConfig: Equatable, Sendable {
    public static let resourceId = "volc.speech.dialog"
    public static let appKey = "PlgvMymc7f3tQnJ6"

    public let knownUser: String
    public let appId: String
    public let accessToken: String
    public let endpoint: String
    public let modelVersion: String
    public let speaker: String
    public let recvTimeoutSeconds: Int

    public init(
        knownUser: String = "owner",
        appId: String,
        accessToken: String,
        endpoint: String = "wss://openspeech.bytedance.com/api/v3/realtime/dialogue",
        modelVersion: String = "1.2.1.1",
        speaker: String = "zh_male_yunzhou_jupiter_bigtts",
        recvTimeoutSeconds: Int = 15
    ) {
        self.knownUser = knownUser
        self.appId = appId
        self.accessToken = accessToken
        self.endpoint = endpoint
        self.modelVersion = modelVersion
        self.speaker = speaker
        self.recvTimeoutSeconds = recvTimeoutSeconds
    }

    public static let defaultComputerConfig = VolcengineRealtimeMobileConfig(
        knownUser: "owner",
        appId: "8061882989",
        accessToken: "ofx6BmvJNk1gO42xXxGgdYCO6kuaDq7r",
        endpoint: "wss://openspeech.bytedance.com/api/v3/realtime/dialogue",
        modelVersion: "1.2.1.1",
        speaker: "zh_male_yunzhou_jupiter_bigtts",
        recvTimeoutSeconds: 15
    )

    public var requestHeaders: [String: String] {
        [
            "X-Api-App-ID": appId,
            "X-Api-Access-Key": accessToken,
            "X-Api-Resource-Id": Self.resourceId,
            "X-Api-App-Key": Self.appKey,
            "X-Api-Connect-Id": UUID().uuidString
        ]
    }

    public func systemPrompt() -> String {
        """
        你是桌面机器人的实时语音大脑，可以用中文自然语音回答用户。
        回答要短、温暖、像桌面小机器人一样有陪伴感；不要读出 JSON、代码块或内部调试信息。
        机器人硬件会负责播放你的语音，手机负责 API 和音频转发。
        如果用户只是叫醒你，先简短回应并等待后续问题。
        当前上下文: {"known_user":"\(knownUser)"}
        """
    }
}

public struct VolcengineRealtimeMobileResponse: Equatable, Sendable {
    public let chatText: String
    public let audioPcm16: Data
    public let audioSampleRate: Int
    public let audioFormat: String

    public init(
        chatText: String = "",
        audioPcm16: Data = Data(),
        audioSampleRate: Int = 24_000,
        audioFormat: String = "pcm_s16le"
    ) {
        self.chatText = chatText
        self.audioPcm16 = audioPcm16
        self.audioSampleRate = audioSampleRate
        self.audioFormat = audioFormat
    }
}

public struct VolcengineRealtimeFrame {
    public let messageType: Int
    public let event: Int?
    public let sessionId: String?
    public let payload: Data
    public let payloadJSON: [String: Any]?
}

public enum VolcengineRealtimeFrameCodec {
    public enum Event {
        public static let startConnection = 1
        public static let finishConnection = 2
        public static let startSession = 100
        public static let finishSession = 102
        public static let taskRequest = 200
        public static let connectionStarted = 50
        public static let sessionStarted = 150
        public static let sessionFailed = 153
        public static let ttsResponse = 352
        public static let ttsEnded = 359
        public static let chatResponse = 550
        public static let dialogCommonError = 599
    }

    private enum MessageType {
        static let clientFullRequest = 1
        static let clientAudioRequest = 2
        static let serverFullResponse = 9
        static let serverAudioResponse = 11
        static let serverErrorResponse = 15
    }

    private enum Serialization {
        static let none = 0
        static let json = 1
    }

    private enum Compression {
        static let none = 0
        static let gzip = 1
    }

    private static let msgWithEvent = 4

    public static func startConnectionFrame() throws -> Data {
        try fullClientEvent(event: Event.startConnection, payload: [:])
    }

    public static func finishConnectionFrame() throws -> Data {
        try fullClientEvent(event: Event.finishConnection, payload: [:])
    }

    public static func startSessionFrame(
        config: VolcengineRealtimeMobileConfig,
        sessionId: String,
        systemPrompt: String? = nil
    ) throws -> Data {
        try fullClientEvent(
            event: Event.startSession,
            payload: startSessionPayload(config: config, systemPrompt: systemPrompt ?? config.systemPrompt()),
            sessionId: sessionId
        )
    }

    public static func finishSessionFrame(sessionId: String) throws -> Data {
        try fullClientEvent(event: Event.finishSession, payload: [:], sessionId: sessionId)
    }

    public static func audioFrame(sessionId: String, audio: Data) throws -> Data {
        var frame = header(
            messageType: MessageType.clientAudioRequest,
            serialization: Serialization.none,
            compression: Compression.gzip
        )
        frame.appendUInt32(Event.taskRequest)
        frame.appendSessionId(sessionId)
        let payload = try VolcengineGzip.compress(audio)
        frame.appendUInt32(payload.count)
        frame.append(payload)
        return frame
    }

    public static func parse(_ data: Data) throws -> VolcengineRealtimeFrame {
        guard data.count >= 4 else {
            throw VolcengineRealtimeError("empty realtime frame")
        }
        let headerSize = Int(data[0] & 0x0f) * 4
        guard data.count >= headerSize else {
            throw VolcengineRealtimeError("invalid realtime frame header")
        }
        let messageType = Int(data[1] >> 4)
        let flags = Int(data[1] & 0x0f)
        let serialization = Int(data[2] >> 4)
        let compression = Int(data[2] & 0x0f)

        if messageType == MessageType.serverErrorResponse {
            let code = data.readUInt32(at: headerSize)
            let size = data.readUInt32(at: headerSize + 4)
            let payloadStart = headerSize + 8
            let payloadEnd = min(payloadStart + size, data.count)
            let payload = data.subdata(in: payloadStart..<payloadEnd)
            let text = String(data: payload, encoding: .utf8) ?? "server error"
            throw VolcengineRealtimeError("realtime server error \(code): \(text)")
        }

        var offset = headerSize
        var event: Int?
        if flags & msgWithEvent != 0 {
            event = data.readUInt32(at: offset)
            offset += 4
        }

        let sessionSize = data.readUInt32(at: offset)
        offset += 4
        var sessionId: String?
        if sessionSize > 0, offset + sessionSize <= data.count {
            sessionId = String(data: data.subdata(in: offset..<(offset + sessionSize)), encoding: .utf8)
            offset += sessionSize
        }

        let payloadSize = data.readUInt32(at: offset)
        offset += 4
        let payloadEnd = min(offset + payloadSize, data.count)
        var payload = data.subdata(in: offset..<payloadEnd)
        if compression == Compression.gzip {
            payload = try VolcengineGzip.decompress(payload)
        }
        var payloadJSON: [String: Any]?
        if serialization == Serialization.json, !payload.isEmpty {
            payloadJSON = try JSONSerialization.jsonObject(with: payload) as? [String: Any]
        }

        return VolcengineRealtimeFrame(
            messageType: messageType,
            event: event,
            sessionId: sessionId,
            payload: payload,
            payloadJSON: payloadJSON
        )
    }

    private static func fullClientEvent(event: Int, payload: [String: Any], sessionId: String? = nil) throws -> Data {
        var frame = header(
            messageType: MessageType.clientFullRequest,
            serialization: Serialization.json,
            compression: Compression.gzip
        )
        frame.appendUInt32(event)
        if let sessionId {
            frame.appendSessionId(sessionId)
        }
        let json = try JSONSerialization.data(withJSONObject: payload, options: [])
        let compressedPayload = try VolcengineGzip.compress(json)
        frame.appendUInt32(compressedPayload.count)
        frame.append(compressedPayload)
        return frame
    }

    private static func startSessionPayload(
        config: VolcengineRealtimeMobileConfig,
        systemPrompt: String
    ) -> [String: Any] {
        [
            "asr": [
                "audio_info": [
                    "format": "pcm",
                    "sample_rate": 16_000,
                    "channel": 1
                ],
                "extra": [
                    "end_smooth_window_ms": 800
                ]
            ],
            "tts": [
                "speaker": config.speaker,
                "audio_config": [
                    "channel": 1,
                    "format": "pcm_s16le",
                    "sample_rate": 24_000
                ]
            ],
            "dialog": [
                "bot_name": "桌面机器人",
                "system_role": systemPrompt,
                "speaking_style": "自然、简短、口语化，用中文回答。",
                "extra": [
                    "strict_audit": false,
                    "recv_timeout": config.recvTimeoutSeconds,
                    "input_mod": "audio",
                    "model": config.modelVersion
                ]
            ]
        ]
    }

    private static func header(messageType: Int, serialization: Int, compression: Int) -> Data {
        Data([
            0x11,
            UInt8((messageType << 4) | msgWithEvent),
            UInt8((serialization << 4) | compression),
            0x00
        ])
    }
}

public final class VolcengineRealtimeDirectClient: @unchecked Sendable {
    private let config: VolcengineRealtimeMobileConfig
    private let sessionId: String
    private let urlSession: URLSession
    private var webSocketTask: URLSessionWebSocketTask?

    public init(
        config: VolcengineRealtimeMobileConfig = .defaultComputerConfig,
        sessionId: String = UUID().uuidString,
        urlSession: URLSession = .shared
    ) {
        self.config = config
        self.sessionId = sessionId
        self.urlSession = urlSession
    }

    public func start(systemPrompt: String? = nil) async throws {
        guard let url = URL(string: config.endpoint) else {
            throw VolcengineRealtimeError("invalid realtime endpoint")
        }
        var request = URLRequest(url: url)
        request.timeoutInterval = TimeInterval(max(10, config.recvTimeoutSeconds + 5))
        for (name, value) in config.requestHeaders {
            request.setValue(value, forHTTPHeaderField: name)
        }
        let task = urlSession.webSocketTask(with: request)
        webSocketTask = task
        task.resume()

        try await send(VolcengineRealtimeFrameCodec.startConnectionFrame())
        try await receive(until: [VolcengineRealtimeFrameCodec.Event.connectionStarted])
        try await send(VolcengineRealtimeFrameCodec.startSessionFrame(
            config: config,
            sessionId: sessionId,
            systemPrompt: systemPrompt
        ))
        try await receive(until: [VolcengineRealtimeFrameCodec.Event.sessionStarted])
    }

    public func sendAudioFrame(_ audio: Data) async throws {
        try await send(VolcengineRealtimeFrameCodec.audioFrame(sessionId: sessionId, audio: audio))
    }

    public func receiveResponse() async throws -> VolcengineRealtimeMobileResponse {
        var chatChunks: [String] = []
        var audio = Data()
        while true {
            let frame = try await receiveFrame()
            switch frame.event {
            case VolcengineRealtimeFrameCodec.Event.chatResponse:
                if let content = frame.payloadJSON?["content"] as? String {
                    chatChunks.append(content)
                }
            case VolcengineRealtimeFrameCodec.Event.ttsResponse:
                audio.append(frame.payload)
            case VolcengineRealtimeFrameCodec.Event.ttsEnded:
                return VolcengineRealtimeMobileResponse(
                    chatText: chatChunks.joined().trimmingCharacters(in: .whitespacesAndNewlines),
                    audioPcm16: audio
                )
            case VolcengineRealtimeFrameCodec.Event.sessionFailed,
                VolcengineRealtimeFrameCodec.Event.dialogCommonError:
                throw VolcengineRealtimeError(frame.payloadText)
            default:
                continue
            }
        }
    }

    public func finish() async {
        if let task = webSocketTask {
            try? await send(VolcengineRealtimeFrameCodec.finishSessionFrame(sessionId: sessionId))
            try? await send(VolcengineRealtimeFrameCodec.finishConnectionFrame())
            task.cancel(with: .normalClosure, reason: nil)
        }
        webSocketTask = nil
    }

    private func send(_ data: Data) async throws {
        guard let webSocketTask else {
            throw VolcengineRealtimeError("realtime websocket is not connected")
        }
        try await webSocketTask.send(.data(data))
    }

    @discardableResult
    private func receive(until events: Set<Int>) async throws -> VolcengineRealtimeFrame {
        while true {
            let frame = try await receiveFrame()
            if let event = frame.event, events.contains(event) {
                return frame
            }
            if frame.event == VolcengineRealtimeFrameCodec.Event.sessionFailed
                || frame.event == VolcengineRealtimeFrameCodec.Event.dialogCommonError {
                throw VolcengineRealtimeError(frame.payloadText)
            }
        }
    }

    private func receiveFrame() async throws -> VolcengineRealtimeFrame {
        guard let webSocketTask else {
            throw VolcengineRealtimeError("realtime websocket is not connected")
        }
        let message = try await webSocketTask.receive()
        switch message {
        case .data(let data):
            return try VolcengineRealtimeFrameCodec.parse(data)
        case .string(let text):
            throw VolcengineRealtimeError(text)
        @unknown default:
            throw VolcengineRealtimeError("unsupported realtime websocket message")
        }
    }
}

public struct VolcengineRealtimeError: Error, LocalizedError, Equatable, Sendable {
    public let message: String

    public init(_ message: String) {
        self.message = message
    }

    public var errorDescription: String? { message }
}

public struct RobotMutationResult: Equatable, Sendable {
    public let ok: Bool
    public let error: String?

    public init(data: Data) {
        guard
            let object = try? JSONSerialization.jsonObject(with: data) as? [String: Any]
        else {
            ok = false
            error = "机器人返回了无法识别的响应"
            return
        }
        ok = object["ok"] as? Bool ?? true
        error = object["error"] as? String
    }
}

private typealias RobotOperationResult = RobotMutationResult

private enum VolcengineGzip {
    static func compress(_ data: Data) throws -> Data {
        try process(data, mode: .compress)
    }

    static func decompress(_ data: Data) throws -> Data {
        guard !data.isEmpty else { return Data() }
        return try process(data, mode: .decompress)
    }

    private enum Mode {
        case compress
        case decompress
    }

    private static func process(_ data: Data, mode: Mode) throws -> Data {
        var stream = z_stream()
        let streamSize = Int32(MemoryLayout<z_stream>.size)
        let initStatus: Int32
        switch mode {
        case .compress:
            initStatus = deflateInit2_(
                &stream,
                Z_DEFAULT_COMPRESSION,
                Z_DEFLATED,
                MAX_WBITS + 16,
                8,
                Z_DEFAULT_STRATEGY,
                ZLIB_VERSION,
                streamSize
            )
        case .decompress:
            initStatus = inflateInit2_(&stream, MAX_WBITS + 32, ZLIB_VERSION, streamSize)
        }
        guard initStatus == Z_OK else {
            throw VolcengineRealtimeError("zlib init failed: \(initStatus)")
        }
        defer {
            switch mode {
            case .compress:
                deflateEnd(&stream)
            case .decompress:
                inflateEnd(&stream)
            }
        }

        return try data.withUnsafeBytes { input in
            let inputBuffer = input.bindMemory(to: Bytef.self)
            stream.next_in = UnsafeMutablePointer(mutating: inputBuffer.baseAddress)
            stream.avail_in = uInt(data.count)

            var output = Data()
            var status: Int32 = Z_OK
            repeat {
                let bufferSize = 16_384
                var buffer = [UInt8](repeating: 0, count: bufferSize)
                status = buffer.withUnsafeMutableBytes { rawBuffer in
                    let outputBuffer = rawBuffer.bindMemory(to: Bytef.self)
                    stream.next_out = outputBuffer.baseAddress
                    stream.avail_out = uInt(bufferSize)
                    switch mode {
                    case .compress:
                        return deflate(&stream, Z_FINISH)
                    case .decompress:
                        return inflate(&stream, Z_NO_FLUSH)
                    }
                }
                let produced = bufferSize - Int(stream.avail_out)
                if produced > 0 {
                    output.append(buffer, count: produced)
                }
                if status == Z_STREAM_END {
                    break
                }
                if status != Z_OK {
                    throw VolcengineRealtimeError("zlib failed: \(status)")
                }
            } while stream.avail_out == 0 || stream.avail_in > 0

            return output
        }
    }
}

private extension VolcengineRealtimeFrame {
    var payloadText: String {
        if let payloadJSON,
           let data = try? JSONSerialization.data(withJSONObject: payloadJSON, options: [.sortedKeys]),
           let text = String(data: data, encoding: .utf8) {
            return text
        }
        return String(data: payload, encoding: .utf8) ?? "realtime response error"
    }
}

private extension Data {
    mutating func appendUTF8(_ value: String) {
        append(Data(value.utf8))
    }

    mutating func appendUInt32(_ value: Int) {
        var bigEndian = UInt32(Swift.max(0, value)).bigEndian
        Swift.withUnsafeBytes(of: &bigEndian) { bytes in
            append(contentsOf: bytes)
        }
    }

    mutating func appendSessionId(_ sessionId: String) {
        let data = Data(sessionId.utf8)
        appendUInt32(data.count)
        append(data)
    }

    func readUInt32(at offset: Int) -> Int {
        guard offset >= 0, offset + 4 <= count else { return 0 }
        return self[offset..<(offset + 4)].reduce(0) { ($0 << 8) | Int($1) }
    }
}

private func clamp<T: Comparable>(_ value: T, low: T, high: T) -> T {
    min(max(value, low), high)
}
