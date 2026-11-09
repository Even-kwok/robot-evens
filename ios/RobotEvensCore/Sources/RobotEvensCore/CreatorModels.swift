import Foundation

public struct NormalizedPoint: Codable, Equatable, Sendable {
    public var x: Double
    public var y: Double

    public init(x: Double, y: Double) {
        self.x = min(max(x, 0), 1)
        self.y = min(max(y, 0), 1)
    }
}

public struct EyeHandleOffset: Codable, Equatable, Sendable {
    public var x: Double
    public var y: Double

    public init(x: Double, y: Double) {
        self.x = min(max(x, -1.5), 1.5)
        self.y = min(max(y, -1.5), 1.5)
    }

    public static prefix func - (offset: EyeHandleOffset) -> EyeHandleOffset {
        EyeHandleOffset(x: -offset.x, y: -offset.y)
    }
}

public struct EyeCurveHandles: Codable, Equatable, Sendable {
    public var incoming: EyeHandleOffset
    public var outgoing: EyeHandleOffset

    public init(incoming: EyeHandleOffset, outgoing: EyeHandleOffset) {
        self.incoming = incoming
        self.outgoing = outgoing
    }
}

public enum SolidEyePresetCollection: String, Codable, CaseIterable, Identifiable, Sendable {
    case batch1
    case batch2
    case batch3

    public var id: String { rawValue }

    public var title: String {
        switch self {
        case .batch1: "采纳第 1 批"
        case .batch2: "采纳第 2 批"
        case .batch3: "采纳第 3 批"
        }
    }

    fileprivate var number: Int {
        switch self {
        case .batch1: 1
        case .batch2: 2
        case .batch3: 3
        }
    }
}

/// The exact 48 pairs extracted from the three user-approved reference sheets.
/// Each raw value resolves to two different bitmaps, so asymmetric eyes stay asymmetric.
public enum SolidEyePreset: String, Codable, CaseIterable, Identifiable, Sendable {
    case batch1_01 = "approved.b1.01"
    case batch1_02 = "approved.b1.02"
    case batch1_03 = "approved.b1.03"
    case batch1_04 = "approved.b1.04"
    case batch1_05 = "approved.b1.05"
    case batch1_06 = "approved.b1.06"
    case batch1_07 = "approved.b1.07"
    case batch1_08 = "approved.b1.08"
    case batch1_09 = "approved.b1.09"
    case batch1_10 = "approved.b1.10"
    case batch1_11 = "approved.b1.11"
    case batch1_12 = "approved.b1.12"
    case batch1_13 = "approved.b1.13"
    case batch1_14 = "approved.b1.14"
    case batch1_15 = "approved.b1.15"
    case batch1_16 = "approved.b1.16"
    case batch2_01 = "approved.b2.01"
    case batch2_02 = "approved.b2.02"
    case batch2_03 = "approved.b2.03"
    case batch2_04 = "approved.b2.04"
    case batch2_05 = "approved.b2.05"
    case batch2_06 = "approved.b2.06"
    case batch2_07 = "approved.b2.07"
    case batch2_08 = "approved.b2.08"
    case batch2_09 = "approved.b2.09"
    case batch2_10 = "approved.b2.10"
    case batch2_11 = "approved.b2.11"
    case batch2_12 = "approved.b2.12"
    case batch2_13 = "approved.b2.13"
    case batch2_14 = "approved.b2.14"
    case batch2_15 = "approved.b2.15"
    case batch2_16 = "approved.b2.16"
    case batch3_01 = "approved.b3.01"
    case batch3_02 = "approved.b3.02"
    case batch3_03 = "approved.b3.03"
    case batch3_04 = "approved.b3.04"
    case batch3_05 = "approved.b3.05"
    case batch3_06 = "approved.b3.06"
    case batch3_07 = "approved.b3.07"
    case batch3_08 = "approved.b3.08"
    case batch3_09 = "approved.b3.09"
    case batch3_10 = "approved.b3.10"
    case batch3_11 = "approved.b3.11"
    case batch3_12 = "approved.b3.12"
    case batch3_13 = "approved.b3.13"
    case batch3_14 = "approved.b3.14"
    case batch3_15 = "approved.b3.15"
    case batch3_16 = "approved.b3.16"

    public var id: String { rawValue }
    public var glyphId: String { rawValue }

    public var collection: SolidEyePresetCollection {
        if rawValue.hasPrefix("approved.b1.") { return .batch1 }
        if rawValue.hasPrefix("approved.b2.") { return .batch2 }
        return .batch3
    }

    public var ordinal: Int {
        Int(rawValue.split(separator: ".").last ?? "1") ?? 1
    }

    public var title: String {
        "第 \(collection.number) 批 · \(String(format: "%02d", ordinal))"
    }

    public var emotionHint: String {
        let row = (ordinal - 1) / 4 + 1
        let column = (ordinal - 1) % 4 + 1
        return "原图第 \(row) 行 · 第 \(column) 组"
    }

    public var suggestedMood: String {
        switch collection {
        case .batch1: "neutral"
        case .batch2: "happy"
        case .batch3: "curious"
        }
    }

    public var recommendedWidthScale: Double {
        ApprovedEyePairBitmapCatalog.recommendedScale(for: rawValue)?.width ?? 0.89
    }

    public var recommendedHeightScale: Double {
        ApprovedEyePairBitmapCatalog.recommendedScale(for: rawValue)?.height ?? 1.49
    }

    public var recommendedEyeSpacing: Int {
        ApprovedEyePairBitmapCatalog.recommendedEyeSpacing(for: rawValue) ?? 64
    }
}

public enum EyeGlyphCategory: String, Codable, CaseIterable, Identifiable, Sendable {
    case money
    case sports
    case music
    case gaming
    case status
    case reaction
    case nature
    case technology
    case travel
    case lifestyle

    public var id: String { rawValue }

    public var title: String {
        switch self {
        case .money: "金钱与奖励"
        case .sports: "运动"
        case .music: "音乐与声音"
        case .gaming: "游戏与能量"
        case .status: "状态符号"
        case .reaction: "情绪与反应"
        case .nature: "自然与天气"
        case .technology: "科技与细节"
        case .travel: "交通与探索"
        case .lifestyle: "生活与物品"
        }
    }
}

public enum EyeGlyphPreset: String, Codable, CaseIterable, Identifiable, Sendable {
    case dollar
    case coin
    case diamond
    case crown
    case soccer
    case basketball
    case baseball
    case tennis
    case football
    case dumbbell
    case trophy
    case musicNote
    case headphones
    case microphone
    case gamepad
    case target
    case dice
    case lightning
    case check
    case cross
    case question
    case exclamation
    case power
    case wifi
    case heart
    case star
    case smile
    case skull
    case fire
    case sparkle
    case angryFace
    case visibility
    case sun
    case moon
    case cloud
    case rain
    case snow
    case waterDrop
    case leaf
    case paw
    case robot
    case fingerprint
    case qrCode
    case chip
    case bluetooth
    case battery
    case camera
    case settings
    case rocket
    case airplane
    case car
    case motorcycle
    case train
    case boat
    case location
    case globe
    case pizza
    case cake
    case coffee
    case iceCream
    case restaurant
    case gift
    case key
    case lock

    public var id: String { rawValue }

    public var title: String {
        switch self {
        case .dollar: "美元"
        case .coin: "金币"
        case .diamond: "钻石"
        case .crown: "皇冠"
        case .soccer: "足球"
        case .basketball: "篮球"
        case .baseball: "棒球"
        case .tennis: "网球"
        case .football: "橄榄球"
        case .dumbbell: "哑铃"
        case .trophy: "奖杯"
        case .musicNote: "音符"
        case .headphones: "耳机"
        case .microphone: "麦克风"
        case .gamepad: "游戏手柄"
        case .target: "目标"
        case .dice: "骰子"
        case .lightning: "闪电"
        case .check: "完成"
        case .cross: "错误"
        case .question: "疑问"
        case .exclamation: "提醒"
        case .power: "电源"
        case .wifi: "网络"
        case .heart: "爱心"
        case .star: "星星"
        case .smile: "大笑脸"
        case .skull: "骷髅"
        case .fire: "火焰"
        case .sparkle: "闪耀"
        case .angryFace: "愤怒脸"
        case .visibility: "注视"
        case .sun: "太阳"
        case .moon: "月亮"
        case .cloud: "云朵"
        case .rain: "下雨"
        case .snow: "雪花"
        case .waterDrop: "水滴"
        case .leaf: "叶子"
        case .paw: "爪印"
        case .robot: "机器人"
        case .fingerprint: "指纹"
        case .qrCode: "二维码"
        case .chip: "芯片"
        case .bluetooth: "蓝牙"
        case .battery: "电池"
        case .camera: "相机"
        case .settings: "齿轮"
        case .rocket: "火箭"
        case .airplane: "飞机"
        case .car: "汽车"
        case .motorcycle: "摩托车"
        case .train: "火车"
        case .boat: "帆船"
        case .location: "定位"
        case .globe: "地球"
        case .pizza: "披萨"
        case .cake: "蛋糕"
        case .coffee: "咖啡"
        case .iceCream: "冰淇淋"
        case .restaurant: "餐具"
        case .gift: "礼物"
        case .key: "钥匙"
        case .lock: "锁"
        }
    }

    public var category: EyeGlyphCategory {
        switch self {
        case .dollar, .coin, .diamond, .crown: .money
        case .soccer, .basketball, .baseball, .tennis, .football, .dumbbell, .trophy: .sports
        case .musicNote, .headphones, .microphone: .music
        case .gamepad, .target, .dice, .lightning: .gaming
        case .check, .cross, .question, .exclamation, .power, .wifi: .status
        case .heart, .star, .smile, .skull, .fire, .sparkle, .angryFace, .visibility: .reaction
        case .sun, .moon, .cloud, .rain, .snow, .waterDrop, .leaf, .paw: .nature
        case .robot, .fingerprint, .qrCode, .chip, .bluetooth, .battery, .camera, .settings: .technology
        case .rocket, .airplane, .car, .motorcycle, .train, .boat, .location, .globe: .travel
        case .pizza, .cake, .coffee, .iceCream, .restaurant, .gift, .key, .lock: .lifestyle
        }
    }

    public var suggestedMood: String {
        switch self {
        case .dollar, .coin, .diamond, .crown, .trophy, .check, .heart, .star, .sparkle, .gift: "love"
        case .soccer, .basketball, .baseball, .tennis, .football, .dumbbell, .lightning,
             .musicNote, .headphones, .microphone, .gamepad, .dice, .smile, .sun, .paw,
             .rocket, .airplane, .car, .motorcycle, .train, .boat, .pizza, .cake, .coffee,
             .iceCream, .restaurant: "happy"
        case .target, .question, .visibility, .moon, .cloud, .rain, .snow, .waterDrop,
             .leaf, .fingerprint, .qrCode, .chip, .camera, .settings, .location, .globe,
             .key, .lock: "thinking"
        case .cross, .exclamation, .skull, .fire, .angryFace: "angry"
        case .power, .wifi, .robot, .bluetooth, .battery: "loading"
        }
    }

    public var recommendedWidthScale: Double {
        switch self {
        case .football, .dumbbell, .gamepad, .wifi, .cloud, .chip, .battery, .car,
             .motorcycle, .train, .boat, .key: 1.15
        case .dollar, .musicNote, .question, .exclamation, .lightning, .fire,
             .waterDrop, .bluetooth, .airplane, .location, .restaurant: 0.82
        default: 0.95
        }
    }

    public var recommendedHeightScale: Double {
        switch self {
        case .dollar, .musicNote, .microphone, .question, .exclamation, .lightning,
             .fire, .waterDrop, .bluetooth, .airplane, .location, .restaurant: 1.18
        case .dumbbell, .wifi, .cloud, .chip, .battery, .car, .motorcycle, .train,
             .boat, .key: 0.82
        default: 1.0
        }
    }

}

public struct RobotFaceStyle: Codable, Equatable, Sendable {
    public var eyeWidthScale: Double
    public var eyeHeightScale: Double
    public var cornerRadius: Int
    public var eyeOffsetX: Int
    public var eyeOffsetY: Int
    public var eyeSpacing: Int
    public var eyeColor: String
    public var glowEnabled: Bool
    public var glowStrength: Int
    public var scanlinesEnabled: Bool
    public var scanlineSpacing: Int
    public var scanlineAlpha: Int

    public init(
        eyeWidthScale: Double = 1,
        eyeHeightScale: Double = 1,
        cornerRadius: Int = 24,
        eyeOffsetX: Int = 0,
        eyeOffsetY: Int = -35,
        eyeSpacing: Int = 112,
        eyeColor: String = "#00f2ff",
        glowEnabled: Bool = false,
        glowStrength: Int = 3,
        scanlinesEnabled: Bool = true,
        scanlineSpacing: Int = 5,
        scanlineAlpha: Int = 20
    ) {
        self.eyeWidthScale = min(max(eyeWidthScale, 0.35), 2.2)
        self.eyeHeightScale = min(max(eyeHeightScale, 0.15), 2.2)
        self.cornerRadius = min(max(cornerRadius, 0), 80)
        self.eyeOffsetX = min(max(eyeOffsetX, -90), 90)
        self.eyeOffsetY = min(max(eyeOffsetY, -90), 90)
        self.eyeSpacing = min(max(eyeSpacing, 48), 220)
        self.eyeColor = eyeColor
        self.glowEnabled = glowEnabled
        self.glowStrength = min(max(glowStrength, 0), 8)
        self.scanlinesEnabled = scanlinesEnabled
        self.scanlineSpacing = min(max(scanlineSpacing, 3), 32)
        self.scanlineAlpha = min(max(scanlineAlpha, 0), 100)
    }
}

public struct RobotEyeLibraryEntry: Decodable, Equatable, Identifiable, Sendable {
    public enum Kind: String, Decodable, Sendable {
        case system
        case custom
    }

    public let id: String
    public let kind: Kind
    public let name: String
    public let baseMood: String
    public let deletable: Bool
    public let editableShape: Bool
    public let hasOverride: Bool
    public let overrideName: String?
    public let glyphId: String?
    public let sweepEnabled: Bool
    public let mirrorRightEye: Bool
    public let smoothingEnabled: Bool
    public let style: RobotFaceStyle
    public let design: EyeShapeDesign?

    private struct WirePoint: Decodable, Equatable {
        let x: Double
        let y: Double
        let smooth: Bool
        let handleInX: Double?
        let handleInY: Double?
        let handleOutX: Double?
        let handleOutY: Double?
    }

    private enum CodingKeys: String, CodingKey {
        case id
        case kind
        case name
        case baseMood
        case deletable
        case editableShape
        case hasOverride
        case overrideName
        case glyphId
        case sweepEnabled
        case mirrorRightEye
        case smoothingEnabled
        case points
        case style
    }

    public init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        id = try container.decode(String.self, forKey: .id)
        kind = try container.decode(Kind.self, forKey: .kind)
        name = try container.decode(String.self, forKey: .name)
        baseMood = try container.decodeIfPresent(String.self, forKey: .baseMood) ?? "neutral"
        deletable = try container.decodeIfPresent(Bool.self, forKey: .deletable) ?? false
        editableShape = try container.decodeIfPresent(Bool.self, forKey: .editableShape) ?? false
        hasOverride = try container.decodeIfPresent(Bool.self, forKey: .hasOverride) ?? false
        overrideName = try container.decodeIfPresent(String.self, forKey: .overrideName)
        glyphId = try container.decodeIfPresent(String.self, forKey: .glyphId)
        sweepEnabled = try container.decodeIfPresent(Bool.self, forKey: .sweepEnabled) ?? false
        mirrorRightEye = try container.decodeIfPresent(Bool.self, forKey: .mirrorRightEye) ?? true
        smoothingEnabled = try container.decodeIfPresent(Bool.self, forKey: .smoothingEnabled) ?? false
        style = try container.decodeIfPresent(RobotFaceStyle.self, forKey: .style) ?? RobotFaceStyle()
        let wirePoints = try container.decodeIfPresent([WirePoint].self, forKey: .points) ?? []
        if let glyphId, !glyphId.isEmpty {
            design = EyeShapeDesign(
                name: overrideName ?? name,
                baseMood: baseMood,
                glyphId: glyphId,
                sweepEnabled: sweepEnabled,
                points: [],
                smoothingEnabled: false,
                style: style
            )
            return
        }
        guard editableShape, wirePoints.count >= 3 else {
            design = nil
            return
        }
        let handles: [EyeCurveHandles?] = wirePoints.map { point in
            guard
                let handleInX = point.handleInX,
                let handleInY = point.handleInY,
                let handleOutX = point.handleOutX,
                let handleOutY = point.handleOutY
            else { return nil }
            return EyeCurveHandles(
                incoming: EyeHandleOffset(x: handleInX, y: handleInY),
                outgoing: EyeHandleOffset(x: handleOutX, y: handleOutY)
            )
        }
        design = EyeShapeDesign(
            name: name,
            baseMood: baseMood,
            sweepEnabled: sweepEnabled,
            mirrorRightEye: mirrorRightEye,
            points: wirePoints.map { NormalizedPoint(x: $0.x, y: $0.y) },
            smoothingEnabled: smoothingEnabled,
            smoothPoints: wirePoints.map(\.smooth),
            controlHandles: handles,
            style: style
        )
    }
}

public struct RobotEyeLibrarySnapshot: Decodable, Equatable, Sendable {
    public let ok: Bool
    public let revision: Int
    public let activeId: String
    public let customCapacity: Int
    public let entries: [RobotEyeLibraryEntry]
    public let savedId: String?
    public let error: String?

    private enum CodingKeys: String, CodingKey {
        case ok
        case revision
        case activeId
        case customCapacity
        case entries
        case savedId
        case error
    }

    public init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        ok = try container.decodeIfPresent(Bool.self, forKey: .ok) ?? false
        revision = try container.decodeIfPresent(Int.self, forKey: .revision) ?? 0
        activeId = try container.decodeIfPresent(String.self, forKey: .activeId) ?? "system.neutral"
        customCapacity = try container.decodeIfPresent(Int.self, forKey: .customCapacity) ?? 0
        entries = try container.decodeIfPresent([RobotEyeLibraryEntry].self, forKey: .entries) ?? []
        savedId = try container.decodeIfPresent(String.self, forKey: .savedId)
        error = try container.decodeIfPresent(String.self, forKey: .error)
    }
}

public struct EyeShapeDesign: Codable, Equatable, Sendable {
    public static let schemaVersion = 5

    public var schemaVersion: Int
    public var name: String
    public var baseMood: String
    public var glyphId: String?
    public var sweepEnabled: Bool
    public var mirrorRightEye: Bool
    public var points: [NormalizedPoint]
    public var smoothingEnabled: Bool
    public var smoothPoints: [Bool]
    public var controlHandles: [EyeCurveHandles?]
    public var style: RobotFaceStyle

    public init(
        name: String = "生气眼形",
        baseMood: String = "angry",
        glyphId: String? = nil,
        sweepEnabled: Bool = false,
        mirrorRightEye: Bool = true,
        points: [NormalizedPoint] = [],
        smoothingEnabled: Bool = true,
        smoothPoints: [Bool]? = nil,
        controlHandles: [EyeCurveHandles?]? = nil,
        style: RobotFaceStyle = RobotFaceStyle()
    ) {
        self.schemaVersion = Self.schemaVersion
        self.name = name
        self.baseMood = baseMood
        self.glyphId = glyphId?.isEmpty == false ? glyphId : nil
        self.sweepEnabled = sweepEnabled
        self.mirrorRightEye = mirrorRightEye
        self.points = Array(points.prefix(24)).map { NormalizedPoint(x: $0.x, y: $0.y) }
        self.smoothingEnabled = smoothingEnabled
        self.smoothPoints = Self.normalizedSmoothPoints(
            smoothPoints,
            count: self.points.count,
            defaultValue: true
        )
        self.controlHandles = Self.normalizedControlHandles(controlHandles, count: self.points.count)
        self.style = style
    }

    public init(preset: SolidEyePreset, style: RobotFaceStyle = RobotFaceStyle()) {
        var style = style
        style.eyeWidthScale = preset.recommendedWidthScale
        style.eyeHeightScale = preset.recommendedHeightScale
        style.eyeSpacing = preset.recommendedEyeSpacing
        self.init(
            name: preset.title,
            baseMood: preset.suggestedMood,
            glyphId: preset.glyphId,
            points: [],
            smoothingEnabled: false,
            smoothPoints: [],
            controlHandles: [],
            style: style
        )
    }

    public init(glyph: EyeGlyphPreset, style: RobotFaceStyle = RobotFaceStyle()) {
        var style = style
        style.eyeWidthScale = glyph.recommendedWidthScale
        style.eyeHeightScale = glyph.recommendedHeightScale
        self.init(
            name: "\(glyph.title)符号眼",
            baseMood: glyph.suggestedMood,
            glyphId: glyph.rawValue,
            points: [],
            smoothingEnabled: false,
            smoothPoints: [],
            controlHandles: [],
            style: style
        )
    }

    public var isGlyph: Bool { glyphId?.isEmpty == false }

    public func isPointSmooth(at index: Int) -> Bool {
        guard smoothingEnabled, smoothPoints.indices.contains(index) else { return false }
        return smoothPoints[index]
    }

    public mutating func replacePoints(_ newPoints: [NormalizedPoint], smoothPoints: [Bool]? = nil) {
        glyphId = nil
        points = Array(newPoints.prefix(24)).map { NormalizedPoint(x: $0.x, y: $0.y) }
        self.smoothPoints = Self.normalizedSmoothPoints(
            smoothPoints,
            count: points.count,
            defaultValue: true
        )
        controlHandles = Array(repeating: nil, count: points.count)
    }

    public mutating func setPointSmooth(_ smooth: Bool, at index: Int) {
        guard points.indices.contains(index) else { return }
        smoothPoints = Self.normalizedSmoothPoints(smoothPoints, count: points.count, defaultValue: true)
        smoothPoints[index] = smooth
    }

    public func effectiveCurveHandles(at index: Int) -> EyeCurveHandles? {
        guard points.indices.contains(index), isPointSmooth(at: index), points.count >= 2 else { return nil }
        if controlHandles.indices.contains(index), let custom = controlHandles[index] {
            return custom
        }
        let previous = points[(index + points.count - 1) % points.count]
        let next = points[(index + 1) % points.count]
        let outgoing = EyeHandleOffset(
            x: (next.x - previous.x) / 6,
            y: (next.y - previous.y) / 6
        )
        return EyeCurveHandles(incoming: -outgoing, outgoing: outgoing)
    }

    public mutating func setLinkedHandle(_ offset: EyeHandleOffset, incoming: Bool, at index: Int) {
        guard points.indices.contains(index) else { return }
        controlHandles = Self.normalizedControlHandles(controlHandles, count: points.count)
        controlHandles[index] = incoming
            ? EyeCurveHandles(incoming: offset, outgoing: -offset)
            : EyeCurveHandles(incoming: -offset, outgoing: offset)
    }

    public mutating func resetHandles(at index: Int) {
        guard points.indices.contains(index) else { return }
        controlHandles = Self.normalizedControlHandles(controlHandles, count: points.count)
        controlHandles[index] = nil
    }

    private enum CodingKeys: String, CodingKey {
        case schemaVersion
        case name
        case baseMood
        case glyphId
        case sweepEnabled
        case mirrorRightEye
        case points
        case smoothingEnabled
        case smoothPoints
        case controlHandles
        case style
    }

    public init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        schemaVersion = Self.schemaVersion
        name = try container.decodeIfPresent(String.self, forKey: .name) ?? "导入眼形"
        baseMood = try container.decodeIfPresent(String.self, forKey: .baseMood) ?? "neutral"
        glyphId = try container.decodeIfPresent(String.self, forKey: .glyphId)
        sweepEnabled = try container.decodeIfPresent(Bool.self, forKey: .sweepEnabled) ?? false
        mirrorRightEye = try container.decodeIfPresent(Bool.self, forKey: .mirrorRightEye) ?? true
        let decodedPoints = try container.decodeIfPresent([NormalizedPoint].self, forKey: .points) ?? []
        points = Array(decodedPoints.prefix(24))
        smoothingEnabled = try container.decodeIfPresent(Bool.self, forKey: .smoothingEnabled) ?? false
        let decodedSmoothPoints = try container.decodeIfPresent([Bool].self, forKey: .smoothPoints)
        smoothPoints = Self.normalizedSmoothPoints(
            decodedSmoothPoints,
            count: points.count,
            defaultValue: smoothingEnabled
        )
        let decodedControlHandles = try container.decodeIfPresent([EyeCurveHandles?].self, forKey: .controlHandles)
        controlHandles = Self.normalizedControlHandles(decodedControlHandles, count: points.count)
        style = try container.decodeIfPresent(RobotFaceStyle.self, forKey: .style) ?? RobotFaceStyle()
    }

    public func encoded(prettyPrinted: Bool = true) throws -> Data {
        let encoder = JSONEncoder()
        if prettyPrinted {
            encoder.outputFormatting = [.prettyPrinted, .sortedKeys, .withoutEscapingSlashes]
        }
        return try encoder.encode(self)
    }

    private static func normalizedSmoothPoints(
        _ values: [Bool]?,
        count: Int,
        defaultValue: Bool
    ) -> [Bool] {
        (0..<count).map { index in
            guard let values, values.indices.contains(index) else { return defaultValue }
            return values[index]
        }
    }

    private static func normalizedControlHandles(
        _ values: [EyeCurveHandles?]?,
        count: Int
    ) -> [EyeCurveHandles?] {
        (0..<count).map { index in
            guard let values, values.indices.contains(index) else { return nil }
            return values[index]
        }
    }
}

public enum EmotionSoundSlot: String, Codable, CaseIterable, Identifiable, Sendable {
    case happy
    case headTouchComboSpin
    case sad
    case angry
    case surprised
    case thinking
    case idleThought
    case shy
    case cute
    case proud
    case confused
    case scared
    case celebrate

    public var id: String { rawValue }

    public var title: String {
        switch self {
        case .happy: "开心点头"
        case .headTouchComboSpin: "摸头疯狂点头"
        case .sad: "伤心低头"
        case .angry: "生气摆头"
        case .surprised: "惊讶后仰"
        case .thinking: "思考低头"
        case .idleThought: "待机嘀咕"
        case .shy: "害羞躲闪"
        case .cute: "撒娇求关注"
        case .proud: "得意抬头"
        case .confused: "困惑歪头"
        case .scared: "紧张后缩"
        case .celebrate: "庆祝欢呼"
        }
    }
}

public struct EmotionSoundSettings: Codable, Equatable, Sendable {
    public var durationMs: Int
    public var speed: Double
    public var pitchSemitones: Double
    public var robotAmount: Double
    public var roughness: Double
    public var gain: Double

    public init(
        durationMs: Int = 900,
        speed: Double = 1,
        pitchSemitones: Double = 0,
        robotAmount: Double = 0.55,
        roughness: Double = 0.25,
        gain: Double = 0.9
    ) {
        self.durationMs = min(max(durationMs, 120), 20_000)
        self.speed = min(max(speed, 0.5), 2)
        self.pitchSemitones = min(max(pitchSemitones, -12), 12)
        self.robotAmount = min(max(robotAmount, 0), 1)
        self.roughness = min(max(roughness, 0), 1)
        self.gain = min(max(gain, 0.1), 1.2)
    }
}

public enum EmotionSoundProcessor {
    public static let sampleRate = 16_000
    public static let maximumSampleCount = sampleRate * 20

    public static func synthesize(
        slot: EmotionSoundSlot,
        durationMs: Int,
        sampleRate: Int = sampleRate
    ) -> [Float] {
        let count = min(max(durationMs, 120) * sampleRate / 1_000, sampleRate * 20)
        guard count > 0 else { return [] }
        var result = [Float](repeating: 0, count: count)
        var noiseState: UInt32 = 0x5f37_59df

        for index in 0..<count {
            let t = Double(index) / Double(sampleRate)
            let progress = Double(index) / Double(max(1, count - 1))
            let envelope = sin(.pi * min(max(progress, 0), 1))
            noiseState = 1_664_525 &* noiseState &+ 1_013_904_223
            let noise = Double(Int32(bitPattern: noiseState)) / Double(Int32.max)
            let value: Double

            switch slot {
            case .happy, .headTouchComboSpin, .proud, .celebrate:
                let chirps = slot == .celebrate ? 3.0 : 2.0
                let local = (progress * chirps).truncatingRemainder(dividingBy: 1)
                let frequency = 420 + 420 * local + (slot == .proud ? -80 : 0)
                value = sin(2 * .pi * frequency * t) * (0.55 + 0.25 * sin(2 * .pi * 7 * t))
            case .angry:
                let frequency = 115 + 45 * sin(2 * .pi * 5.5 * t)
                value = 0.65 * sin(2 * .pi * frequency * t) + 0.28 * noise
            case .sad:
                let frequency = 360 - 230 * progress
                value = sin(2 * .pi * frequency * t) * (0.75 - 0.30 * progress)
            case .surprised, .scared:
                let frequency = (slot == .scared ? 520 : 320) + (slot == .scared ? 650 : 980) * progress
                value = sin(2 * .pi * frequency * t) * (slot == .scared ? 0.55 + 0.2 * sin(2 * .pi * 14 * t) : 0.8)
            case .thinking, .confused, .idleThought:
                let pulseRate = slot == .idleThought ? 4.0 : 2.5
                let local = (progress * pulseRate).truncatingRemainder(dividingBy: 1)
                let frequency = (slot == .confused ? 260 : 190) + 240 * local
                value = sin(2 * .pi * frequency * t) * (local < 0.58 ? 0.72 : 0.12)
            case .shy, .cute:
                let frequency = 520 + 120 * sin(2 * .pi * (slot == .cute ? 5 : 3) * t)
                value = sin(2 * .pi * frequency * t) * (0.62 + 0.18 * sin(2 * .pi * 9 * t))
            }
            result[index] = Float(value * envelope)
        }
        return result
    }

    public static func process(_ input: [Float], settings: EmotionSoundSettings) -> [Float] {
        guard !input.isEmpty else { return [] }
        let speed = min(max(settings.speed, 0.5), 2)
        let outputCount = min(max(1, Int(Double(input.count) / speed)), maximumSampleCount)
        var sped = [Float](repeating: 0, count: outputCount)
        for index in 0..<outputCount {
            sped[index] = interpolated(input, at: Double(index) * speed)
        }

        let shifted = granularPitchShift(sped, semitones: settings.pitchSemitones)
        let robotAmount = min(max(settings.robotAmount, 0), 1)
        let roughness = min(max(settings.roughness, 0), 1)
        let hold = max(1, 1 + Int((roughness * 7).rounded()))
        let carrierFrequency = 28 + robotAmount * 78
        let levels = max(8, Int(pow(2, 8 - roughness * 4)))
        var result = [Float](repeating: 0, count: shifted.count)
        var held: Float = 0

        for index in shifted.indices {
            if index % hold == 0 { held = shifted[index] }
            let dry = Double(shifted[index])
            let crushed = (Double(held) * Double(levels)).rounded() / Double(levels)
            let ring = sin(2 * .pi * carrierFrequency * Double(index) / Double(sampleRate))
            let robotic = crushed * (0.58 + 0.42 * ring)
            let mixed = dry * (1 - robotAmount) + robotic * robotAmount
            result[index] = Float(tanh(mixed * (1.2 + roughness * 2.6)) * settings.gain)
        }
        applyEdgeFadeAndNormalize(&result)
        return result
    }

    public static func resample(_ input: [Float], sourceRate: Double, targetRate: Int = sampleRate) -> [Float] {
        guard !input.isEmpty, sourceRate > 0, targetRate > 0 else { return [] }
        let count = min(max(1, Int((Double(input.count) * Double(targetRate) / sourceRate).rounded())), targetRate * 20)
        return (0..<count).map { index in
            interpolated(input, at: Double(index) * sourceRate / Double(targetRate))
        }
    }

    public static func rawU8Data(from samples: [Float]) -> Data {
        Data(samples.prefix(maximumSampleCount).map { sample in
            let normalized = min(max(Double(sample), -1), 1)
            return UInt8(min(max(Int(((normalized + 1) * 127.5).rounded()), 0), 255))
        })
    }

    public static func wavData(fromRawU8 raw: Data, sampleRate: Int = sampleRate) -> Data {
        let payloadSize = UInt32(raw.count)
        var data = Data()
        data.appendASCII("RIFF")
        data.appendLittleEndian(UInt32(36) + payloadSize)
        data.appendASCII("WAVEfmt ")
        data.appendLittleEndian(UInt32(16))
        data.appendLittleEndian(UInt16(1))
        data.appendLittleEndian(UInt16(1))
        data.appendLittleEndian(UInt32(sampleRate))
        data.appendLittleEndian(UInt32(sampleRate))
        data.appendLittleEndian(UInt16(1))
        data.appendLittleEndian(UInt16(8))
        data.appendASCII("data")
        data.appendLittleEndian(payloadSize)
        data.append(raw)
        return data
    }

    public static func waveform(_ samples: [Float], bucketCount: Int = 96) -> [Double] {
        guard !samples.isEmpty, bucketCount > 0 else { return [] }
        return (0..<bucketCount).map { bucket in
            let start = bucket * samples.count / bucketCount
            let end = max(start + 1, (bucket + 1) * samples.count / bucketCount)
            return samples[start..<min(end, samples.count)].reduce(0) { max($0, Double(abs($1))) }
        }
    }

    private static func granularPitchShift(_ samples: [Float], semitones: Double) -> [Float] {
        let semitones = min(max(semitones, -12), 12)
        guard abs(semitones) > 0.05, samples.count > 64 else { return samples }
        let ratio = pow(2, semitones / 12)
        let grainSize = min(320, max(64, samples.count / 4))
        let hop = max(24, grainSize / 4)
        var output = [Double](repeating: 0, count: samples.count)
        var weights = [Double](repeating: 0, count: samples.count)
        let sourceSpan = max(1, samples.count - grainSize - 1)

        for outputStart in stride(from: 0, to: samples.count, by: hop) {
            let sourceStart = Int((Double(outputStart) * ratio).truncatingRemainder(dividingBy: Double(sourceSpan)))
            for offset in 0..<grainSize where outputStart + offset < samples.count {
                let sourcePosition = Double(sourceStart) + Double(offset) * ratio
                let sample = Double(interpolated(samples, at: sourcePosition.truncatingRemainder(dividingBy: Double(samples.count - 1))))
                let window = 0.5 - 0.5 * cos(2 * .pi * Double(offset) / Double(max(1, grainSize - 1)))
                output[outputStart + offset] += sample * window
                weights[outputStart + offset] += window
            }
        }
        return output.indices.map { index in
            weights[index] > 0.0001 ? Float(output[index] / weights[index]) : samples[index]
        }
    }

    private static func interpolated(_ samples: [Float], at position: Double) -> Float {
        let position = min(max(position, 0), Double(max(0, samples.count - 1)))
        let lower = Int(position.rounded(.down))
        let upper = min(lower + 1, samples.count - 1)
        let fraction = Float(position - Double(lower))
        return samples[lower] + (samples[upper] - samples[lower]) * fraction
    }

    private static func applyEdgeFadeAndNormalize(_ samples: inout [Float]) {
        guard !samples.isEmpty else { return }
        let peak = samples.reduce(Float(0)) { max($0, abs($1)) }
        let normalization: Float = peak > 0.98 ? 0.98 / peak : 1
        let fadeCount = min(sampleRate / 100, samples.count / 2)
        for index in samples.indices {
            var gain = normalization
            if index < fadeCount { gain *= Float(index) / Float(max(1, fadeCount)) }
            let tail = samples.count - index - 1
            if tail < fadeCount { gain *= Float(tail) / Float(max(1, fadeCount)) }
            samples[index] *= gain
        }
    }
}

private extension Data {
    mutating func appendASCII(_ text: String) {
        append(contentsOf: text.utf8)
    }

    mutating func appendLittleEndian<T: FixedWidthInteger>(_ value: T) {
        var value = value.littleEndian
        Swift.withUnsafeBytes(of: &value) { append(contentsOf: $0) }
    }
}
