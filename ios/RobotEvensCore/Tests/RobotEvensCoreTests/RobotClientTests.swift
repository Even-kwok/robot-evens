import XCTest
@testable import RobotEvensCore

final class RobotClientTests: XCTestCase {
    func testPingRequestUsesFirmwarePingEndpoint() {
        let request = RobotEndpointBuilder.ping()

        XCTAssertEqual(request.method, "GET")
        XCTAssertEqual(request.path, "/api/ping")
        XCTAssertTrue(request.body.isEmpty)
    }

    func testPoseRequestUsesFirmwareMotionEndpoint() {
        let request = RobotEndpointBuilder.pose(ServoPose(yaw: 300, pitch: 180, speed: 260))

        XCTAssertEqual(request.method, "POST")
        XCTAssertEqual(request.path, "/api/motion/pose")
        XCTAssertEqual(request.body, [
            "yaw": .int(300),
            "pitch": .int(180),
            "speed": .int(260)
        ])
    }

    func testFaceRequestUsesFirmwareFaceEndpoint() {
        let request = RobotEndpointBuilder.face(mood: "happy", color: "#00f2ff")

        XCTAssertEqual(request.method, "POST")
        XCTAssertEqual(request.path, "/api/face/set")
        XCTAssertEqual(request.body, [
            "mood": .string("happy"),
            "color": .string("#00f2ff")
        ])
    }

    func testFaceStyleAndCustomShapeRequestsReuseFirmwareEditorFields() {
        let design = EyeShapeDesign(
            sweepEnabled: true,
            points: [
                NormalizedPoint(x: 0.05, y: 0.2),
                NormalizedPoint(x: 0.50, y: 0.05),
                NormalizedPoint(x: 0.95, y: 0.3),
                NormalizedPoint(x: 0.78, y: 0.85),
                NormalizedPoint(x: 0.15, y: 0.8)
            ],
            smoothingEnabled: true,
            smoothPoints: [false, true, true, false, false],
            style: RobotFaceStyle(eyeWidthScale: 1.25, eyeSpacing: 104, eyeColor: "#ff3300")
        )

        let style = RobotEndpointBuilder.faceStyle(design.style, mood: design.baseMood)
        let shape = RobotEndpointBuilder.customEyeShape(design)

        XCTAssertEqual(style.path, "/api/face/style")
        XCTAssertEqual(style.body["mood"], .string("angry"))
        XCTAssertEqual(style.body["eyeWidthScale"], .double(1.25))
        XCTAssertEqual(style.body["eyeSpacing"], .int(104))
        XCTAssertEqual(style.body["eyeColor"], .string("#ff3300"))
        XCTAssertEqual(shape.path, "/api/face/shape")
        XCTAssertEqual(shape.body["enabled"], .bool(true))
        XCTAssertEqual(shape.body["mirrorRightEye"], .bool(true))
        XCTAssertEqual(shape.body["smoothingEnabled"], .bool(true))
        XCTAssertEqual(shape.body["sweepEnabled"], .bool(true))
        guard case .array(let points) = shape.body["points"] else {
            return XCTFail("missing custom eye points")
        }
        XCTAssertEqual(points.count, 5)
        guard let first = points.first, case .object(let firstPoint) = first else {
            return XCTFail("missing first custom eye point")
        }
        XCTAssertEqual(firstPoint["smooth"], .bool(false))
    }

    func testCustomEyePreviewUsesTemporaryOverlayWithoutChangingBaseMood() {
        var design = EyeShapeDesign(
            baseMood: "angry",
            sweepEnabled: true,
            points: [
                NormalizedPoint(x: 0.05, y: 0.5),
                NormalizedPoint(x: 0.25, y: 0.1),
                NormalizedPoint(x: 0.65, y: 0.05),
                NormalizedPoint(x: 0.95, y: 0.4),
                NormalizedPoint(x: 0.55, y: 0.95)
            ],
            smoothingEnabled: true,
            smoothPoints: [true, true, true, true, true],
            style: RobotFaceStyle(eyeWidthScale: 1.4, eyeSpacing: 118, eyeColor: "#ff5500")
        )
        design.setLinkedHandle(EyeHandleOffset(x: 0.22, y: -0.09), incoming: false, at: 3)

        let preview = RobotEndpointBuilder.customEyePreview(
            design,
            durationMs: 30_000,
            blinkEnabled: true,
            blinkNow: true
        )

        XCTAssertEqual(preview.method, "POST")
        XCTAssertEqual(preview.path, "/api/face/preview")
        XCTAssertEqual(preview.body["durationMs"], .int(15_000))
        XCTAssertEqual(preview.body["blinkEnabled"], .bool(true))
        XCTAssertEqual(preview.body["blinkNow"], .bool(true))
        XCTAssertEqual(preview.body["eyeWidthScale"], .double(1.4))
        XCTAssertEqual(preview.body["eyeSpacing"], .int(118))
        XCTAssertEqual(preview.body["eyeColor"], .string("#ff5500"))
        XCTAssertEqual(preview.body["smoothingEnabled"], .bool(true))
        XCTAssertEqual(preview.body["sweepEnabled"], .bool(true))
        guard case .array(let previewPoints) = preview.body["points"],
              case .object(let smoothPoint) = previewPoints[3] else {
            return XCTFail("missing preview smoothing data")
        }
        XCTAssertEqual(smoothPoint["smooth"], .bool(true))
        XCTAssertEqual(smoothPoint["handleInX"], .double(-0.22))
        XCTAssertEqual(smoothPoint["handleInY"], .double(0.09))
        XCTAssertEqual(smoothPoint["handleOutX"], .double(0.22))
        XCTAssertEqual(smoothPoint["handleOutY"], .double(-0.09))
        XCTAssertNil(preview.body["mood"])
        XCTAssertEqual(RobotEndpointBuilder.stopCustomEyePreview().path, "/api/face/preview/stop")
        XCTAssertEqual(RobotEndpointBuilder.customEyePreviewStatus().path, "/api/face/preview/status")
    }

    func testEyeLibraryRequestsTargetOneFirmwareMoodSlotWithRevisionGuard() {
        let design = EyeShapeDesign(
            name: "生气替换",
            baseMood: "angry",
            sweepEnabled: true,
            points: [
                NormalizedPoint(x: 0.05, y: 0.2),
                NormalizedPoint(x: 0.55, y: 0.05),
                NormalizedPoint(x: 0.95, y: 0.4),
                NormalizedPoint(x: 0.70, y: 0.9),
                NormalizedPoint(x: 0.15, y: 0.8)
            ],
            style: RobotFaceStyle(eyeColor: "#ff3300")
        )

        let list = RobotEndpointBuilder.eyeLibrary()
        let save = RobotEndpointBuilder.saveCustomEyeLibraryEntry(
            id: "system.angry",
            design: design,
            expectedRevision: 7
        )
        let activate = RobotEndpointBuilder.activateEyeLibraryEntry(id: "system.angry", expectedRevision: 8)
        let delete = RobotEndpointBuilder.deleteEyeLibraryEntry(id: "system.angry", expectedRevision: 8)

        XCTAssertEqual(list, RobotRequest(method: "GET", path: "/api/face/library"))
        XCTAssertEqual(save.path, "/api/face/library/save")
        XCTAssertEqual(save.body["id"], .string("system.angry"))
        XCTAssertEqual(save.body["baseMood"], .string("angry"))
        XCTAssertEqual(save.body["expectedRevision"], .int(7))
        XCTAssertEqual(save.body["kind"], .string("custom"))
        XCTAssertEqual(save.body["sweepEnabled"], .bool(true))
        XCTAssertEqual(activate.path, "/api/face/library/activate")
        XCTAssertEqual(delete.path, "/api/face/library/delete")
    }

    func testGlyphEyeRequestsUseFirmwareGlyphModeWithoutFakePathPoints() {
        var design = EyeShapeDesign(glyph: .soccer)
        design.baseMood = "happy"

        let save = RobotEndpointBuilder.saveCustomEyeLibraryEntry(
            id: "system.happy",
            design: design,
            expectedRevision: 9
        )
        let shape = RobotEndpointBuilder.customEyeShape(design)
        let preview = RobotEndpointBuilder.customEyePreview(design, blinkEnabled: true)

        XCTAssertEqual(save.body["kind"], .string("glyph"))
        XCTAssertEqual(save.body["glyphId"], .string("soccer"))
        XCTAssertNil(save.body["points"])
        XCTAssertEqual(shape.body["glyphId"], .string("soccer"))
        XCTAssertNil(shape.body["points"])
        XCTAssertEqual(preview.body["glyphId"], .string("soccer"))
        XCTAssertNil(preview.body["points"])
        XCTAssertEqual(preview.body["blinkEnabled"], .bool(true))

        let approvedPair = EyeShapeDesign(preset: .batch2_04)
        let approvedPreview = RobotEndpointBuilder.customEyePreview(approvedPair)
        XCTAssertEqual(approvedPreview.body["glyphId"], .string("approved.b2.04"))
        XCTAssertNil(approvedPreview.body["points"])
    }

    func testClientEyePreviewSendsOnlyPreviewAndStopRequests() async throws {
        let transport = RecordingRobotHTTPTransport(responseData: Data(#"{"ok":true}"#.utf8))
        let client = RobotClient(baseURL: URL(string: "http://robot.local")!, transport: transport)
        let design = EyeShapeDesign(preset: .batch1_01)

        let previewResult = try await client.previewEyeShapeDesign(design)
        let stopResult = try await client.stopEyeShapePreview()
        XCTAssertTrue(previewResult.ok)
        XCTAssertTrue(stopResult.ok)

        XCTAssertEqual(transport.requests, [
            RobotEndpointBuilder.customEyePreview(design),
            RobotEndpointBuilder.stopCustomEyePreview()
        ])
        XCTAssertFalse(transport.requests.contains { $0.path == "/api/face/style" })
        XCTAssertFalse(transport.requests.contains { $0.path == "/api/face/shape" })
    }

    func testEmotionSoundSlotRequestsUseRuntimeSlotEndpoints() {
        XCTAssertEqual(
            RobotEndpointBuilder.emotionSoundSlotSet(slot: .angry, enabled: true),
            RobotRequest(
                method: "POST",
                path: "/api/emotion/sound-slot/set",
                body: ["slot": .string("angry"), "enabled": .bool(true)]
            )
        )
        XCTAssertEqual(
            RobotEndpointBuilder.emotionSoundSlotPlay(slot: .angry),
            RobotRequest(
                method: "POST",
                path: "/api/emotion/sound-slot/play",
                body: ["slot": .string("angry"), "durationMs": .int(0)]
            )
        )
    }

    func testRGBRequestUsesFirmwareRGBEndpointAndClampsBrightness() {
        let request = RobotEndpointBuilder.rgb(
            color: "#00ff66",
            side: "left",
            brightness: 300,
            breathing: true,
            marquee: false,
            rainbow: false,
            preset: "manual"
        )

        XCTAssertEqual(request.method, "POST")
        XCTAssertEqual(request.path, "/api/rgb")
        XCTAssertEqual(request.body, [
            "color": .string("#00ff66"),
            "side": .string("left"),
            "brightness": .int(255),
            "breathing": .bool(true),
            "marquee": .bool(false),
            "rainbow": .bool(false),
            "preset": .string("manual")
        ])
    }

    func testTemporaryRGBRequestSkipsStandbyCapture() {
        let request = RobotEndpointBuilder.rgb(
            color: "#ff3366",
            brightness: 180,
            temporary: true
        )

        XCTAssertEqual(request.method, "POST")
        XCTAssertEqual(request.path, "/api/rgb")
        XCTAssertEqual(request.body, [
            "color": .string("#ff3366"),
            "brightness": .int(180),
            "temporary": .bool(true)
        ])
    }

    func testRestoreStandbyRGBRequestUsesFirmwareEndpoint() {
        let request = RobotEndpointBuilder.restoreStandbyRGB()

        XCTAssertEqual(request.method, "POST")
        XCTAssertEqual(request.path, "/api/rgb/restore")
        XCTAssertTrue(request.body.isEmpty)
    }

    func testMicSampleRequestUsesFirmwareMicEndpointAndClampsSamplingSettings() {
        let request = RobotEndpointBuilder.micSample(samples: 20, sampleRate: 48_000)

        XCTAssertEqual(request.method, "POST")
        XCTAssertEqual(request.path, "/api/mic/sample")
        XCTAssertEqual(request.body, [
            "samples": .int(64),
            "sampleRate": .int(24_000)
        ])
    }

    func testMicSampleResponseClampsLevel() throws {
        let data = Data(#"{"ok":true,"micEnabled":true,"recordAccepted":true,"level":1.7}"#.utf8)

        let sample = try JSONDecoder().decode(RobotMicSample.self, from: data)

        XCTAssertEqual(sample.level, 1)
        XCTAssertTrue(sample.ok)
        XCTAssertTrue(sample.micEnabled)
    }

    func testSoundRequestInvokesAssetById() {
        let request = RobotEndpointBuilder.invokeAsset("sound.cute_dance_01")

        XCTAssertEqual(request.method, "POST")
        XCTAssertEqual(request.path, "/api/asset/invoke")
        XCTAssertEqual(request.body, ["assetId": .string("sound.cute_dance_01")])
    }

    func testHomeRequestUsesFirmwareHomeEndpoint() {
        let request = RobotEndpointBuilder.home(speed: 300)

        XCTAssertEqual(request.method, "POST")
        XCTAssertEqual(request.path, "/api/motion/home")
        XCTAssertEqual(request.body, ["speed": .int(300)])
    }

    func testMotionStreamStatusUsesFirmwareStreamEndpoint() {
        let request = RobotEndpointBuilder.motionStreamStatus()

        XCTAssertEqual(request.method, "GET")
        XCTAssertEqual(request.path, "/api/motion/stream/status")
        XCTAssertTrue(request.body.isEmpty)
    }

    func testRobotRealtimeVoicePlaybackConvertsApiPcmAndUsesMumbleEndpoints() async throws {
        let transport = RecordingRobotHTTPTransport(
            responseData: Data(#"{"ok":true}"#.utf8)
        )
        let client = RobotClient(
            baseURL: URL(string: "http://robot.local")!,
            transport: transport
        )

        let response = try await client.playRealtimeVoice(
            Data([0x00, 0x00, 0xff, 0x7f, 0x00, 0x80]),
            sampleRate: 16_000
        )

        XCTAssertTrue(response.ok)
        XCTAssertEqual(response.sourceSampleRate, 16_000)
        XCTAssertEqual(response.sampleRate, 16_000)
        XCTAssertEqual(response.sourceBytes, 6)
        XCTAssertEqual(response.uploadedBytes, 3)
        XCTAssertEqual(transport.uploads.map(\.path), ["/api/mumble/upload"])
        XCTAssertEqual(transport.uploads.map(\.fieldName), ["file"])
        XCTAssertEqual(transport.uploads.map(\.filename), ["realtime-api.raw"])
        XCTAssertEqual(transport.uploads.map(\.data), [Data([0x80, 0xff, 0x00])])
        XCTAssertEqual(transport.requests, [
            RobotEndpointBuilder.mumbleSet(enabled: true),
            RobotEndpointBuilder.mumblePlay(mode: "full")
        ])
        XCTAssertEqual(transport.baseURLs, [
            URL(string: "http://robot.local")!,
            URL(string: "http://robot.local")!,
            URL(string: "http://robot.local")!
        ])
    }

    func testEmotionSoundUploadTargetsSelectedFirmwareSlotAndPlaysIt() async throws {
        let transport = RecordingRobotHTTPTransport(responseData: Data(#"{"ok":true}"#.utf8))
        let client = RobotClient(baseURL: URL(string: "http://robot.local")!, transport: transport)

        let result = try await client.uploadEmotionSoundSlot(.sad, rawU8Audio: Data([1, 2, 3]))

        XCTAssertTrue(result.ok)
        XCTAssertEqual(transport.uploads.map(\.path), ["/api/emotion/sound-slot/upload?slot=sad"])
        XCTAssertEqual(transport.uploads.map(\.filename), ["sad.raw"])
        XCTAssertEqual(transport.requests, [
            RobotEndpointBuilder.emotionSoundSlotSet(slot: .sad),
            RobotEndpointBuilder.emotionSoundSlotPlay(slot: .sad)
        ])
    }

    func testVolcengineMobileConfigDefaultsUseComputerRealtimeCredentials() {
        let config = VolcengineRealtimeMobileConfig.defaultComputerConfig

        XCTAssertEqual(config.knownUser, "owner")
        XCTAssertEqual(config.appId, "8061882989")
        XCTAssertEqual(config.endpoint, "wss://openspeech.bytedance.com/api/v3/realtime/dialogue")
        XCTAssertEqual(config.modelVersion, "1.2.1.1")
        XCTAssertEqual(config.speaker, "zh_male_yunzhou_jupiter_bigtts")
        XCTAssertGreaterThan(config.accessToken.count, 20)
        XCTAssertEqual(config.recvTimeoutSeconds, 15)
    }

    func testVolcengineRealtimeFrameCodecBuildsGzipAudioTaskFrame() throws {
        let frame = try VolcengineRealtimeFrameCodec.audioFrame(
            sessionId: "session-1",
            audio: Data([1, 2, 3])
        )

        let parsed = try VolcengineRealtimeFrameCodec.parse(frame)

        XCTAssertEqual(parsed.event, VolcengineRealtimeFrameCodec.Event.taskRequest)
        XCTAssertEqual(parsed.sessionId, "session-1")
        XCTAssertEqual(parsed.payload, Data([1, 2, 3]))
    }
}

private final class RecordingRobotHTTPTransport: RobotHTTPTransport, @unchecked Sendable {
    struct Upload: Equatable {
        let path: String
        let fieldName: String
        let filename: String
        let data: Data
        let contentType: String
    }

    private(set) var requests: [RobotRequest] = []
    private(set) var uploads: [Upload] = []
    private(set) var baseURLs: [URL] = []
    private let responseData: Data

    init(responseData: Data) {
        self.responseData = responseData
    }

    func send(_ request: RobotRequest, baseURL: URL) async throws -> Data {
        requests.append(request)
        baseURLs.append(baseURL)
        return responseData
    }

    func upload(
        path: String,
        fieldName: String,
        filename: String,
        data: Data,
        contentType: String,
        baseURL: URL
    ) async throws -> Data {
        uploads.append(Upload(
            path: path,
            fieldName: fieldName,
            filename: filename,
            data: data,
            contentType: contentType
        ))
        baseURLs.append(baseURL)
        return responseData
    }
}
