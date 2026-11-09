import PhotosUI
import SwiftUI
import UniformTypeIdentifiers
import Vision

@MainActor
final class ExpressionStudioModel: ObservableObject {
    @Published var design = EyeShapeDesign(preset: SolidEyePreset.batch1_01)
    @Published private(set) var eyeLibraryEntries: [RobotEyeLibraryEntry] = []
    @Published var selectedEyeLibraryId = ""
    @Published private(set) var activeEyeLibraryId = "system.neutral"
    @Published private(set) var eyeLibraryRevision = 0
    @Published private(set) var customEyeCapacity = 0
    @Published private(set) var isEditingOverrideDraft = false
    @Published private(set) var selectedPointIndex = 0
    @Published private(set) var referenceImage: UIImage?
    @Published private(set) var statusText = "先选择机器人表情槽位，再从三批实心眼型或符号库直接选用"
    @Published private(set) var isBusy = false
    @Published private(set) var isPreviewing = false
    @Published private(set) var previewBlinkEnabled = true

    private var previewRefreshTask: Task<Void, Never>?
    private var previewKeepAliveTask: Task<Void, Never>?
    private var previewRefreshPending = false
    private var previewLastSendAt: ContinuousClock.Instant?

    var selectedEyeLibraryEntry: RobotEyeLibraryEntry? {
        eyeLibraryEntries.first { $0.id == selectedEyeLibraryId }
    }

    var canEditSelectedShape: Bool {
        isEditingOverrideDraft ||
            selectedEyeLibraryEntry?.editableShape == true ||
            selectedEyeLibraryEntry?.glyphId?.isEmpty == false
    }

    var canDeleteSelectedEye: Bool {
        selectedEyeLibraryEntry?.deletable ?? false
    }

    var customEyeCount: Int {
        eyeLibraryEntries.filter(\.hasOverride).count
    }

    func syncEyeLibrary(robotBaseURLString: String, preferredId: String? = nil) async {
        guard let client = robotClient(robotBaseURLString: robotBaseURLString) else {
            statusText = "机器人地址无效"
            return
        }
        isBusy = true
        defer { isBusy = false }
        do {
            let snapshot = try await client.eyeLibrary()
            guard snapshot.ok else {
                statusText = "机器人拒绝同步：\(snapshot.error ?? "未知错误")"
                return
            }
            applyLibrarySnapshot(snapshot, preferredId: preferredId)
            statusText = "已从机器人同步 \(snapshot.entries.count) 个眼形；机器人库为准"
        } catch {
            statusText = "同步机器人眼睛库失败：\(error.localizedDescription)"
        }
    }

    func selectEyeLibraryEntry(_ id: String) {
        selectedEyeLibraryId = id
        guard let entry = selectedEyeLibraryEntry else { return }
        loadLibraryEntryIntoEditor(entry)
        statusText = entry.hasOverride
            ? "已载入“\(libraryDisplayName(entry))”的手机自定义替换眼形"
            : "这是固件程序化眼形；可从下方三批新实心眼型直接选一个替换"
    }

    func createCustomEye(fromSelected: Bool = false) {
        guard customEyeCapacity == 0 || customEyeCount < customEyeCapacity else {
            statusText = "机器人自定义眼形库已满（\(customEyeCapacity) 个）"
            return
        }
        let sourceStyle = design.style
        guard let selectedEntry = selectedEyeLibraryEntry else {
            statusText = "请先选择要替换的机器人表情"
            return
        }
        var next = fromSelected && canEditSelectedShape
            ? design
            : EyeShapeDesign(preset: .batch1_01, style: sourceStyle)
        next.name = "新眼形"
        next.baseMood = selectedEntry.baseMood
        design = next
        isEditingOverrideDraft = true
        selectedPointIndex = 0
        statusText = "正在为“\(libraryDisplayName(selectedEntry))”制作替换眼形；保存后只有这个表情会改变"
    }

    func applySolidPreset(_ preset: SolidEyePreset) {
        guard let selectedEntry = selectedEyeLibraryEntry else {
            statusText = "请先选择要替换的机器人表情槽位"
            return
        }
        let createsNewOverride = !isEditingOverrideDraft && !selectedEntry.hasOverride
        if createsNewOverride, customEyeCapacity > 0, customEyeCount >= customEyeCapacity {
            statusText = "机器人自定义眼形库已满（\(customEyeCapacity) 个）"
            return
        }

        let sweepEnabled = design.sweepEnabled
        var next = EyeShapeDesign(preset: preset, style: design.style)
        next.baseMood = selectedEntry.baseMood
        next.sweepEnabled = sweepEnabled
        design = next
        isEditingOverrideDraft = true
        selectedPointIndex = 0
        statusText = "已为“\(libraryDisplayName(selectedEntry))”选用“\(preset.title)”；可选扫光，再调眼距、宽高、位置和颜色"
    }

    func applyGlyphPreset(_ preset: EyeGlyphPreset) {
        guard let selectedEntry = selectedEyeLibraryEntry else {
            statusText = "请先选择要替换的机器人表情槽位"
            return
        }
        let createsNewOverride = !isEditingOverrideDraft && !selectedEntry.hasOverride
        if createsNewOverride, customEyeCapacity > 0, customEyeCount >= customEyeCapacity {
            statusText = "机器人自定义眼形库已满（\(customEyeCapacity) 个）"
            return
        }

        var next = EyeShapeDesign(glyph: preset, style: design.style)
        next.baseMood = selectedEntry.baseMood
        design = next
        isEditingOverrideDraft = true
        selectedPointIndex = 0
        statusText = "已为“\(libraryDisplayName(selectedEntry))”选用“\(preset.title)”符号眼；可调大小、眼距、位置和颜色，再实时预览并保存"
    }

    func saveSelectedEye(robotBaseURLString: String) async {
        guard let client = robotClient(robotBaseURLString: robotBaseURLString) else {
            statusText = "机器人地址无效"
            return
        }
        isBusy = true
        defer { isBusy = false }
        do {
            let snapshot: RobotEyeLibrarySnapshot
            if let entry = selectedEyeLibraryEntry, !isEditingOverrideDraft, !entry.hasOverride {
                snapshot = try await client.saveSystemEyeLibraryEntry(
                    id: entry.id,
                    mood: entry.baseMood,
                    style: design.style,
                    expectedRevision: eyeLibraryRevision
                )
            } else {
                snapshot = try await client.saveCustomEyeLibraryEntry(
                    id: selectedEyeLibraryEntry?.id,
                    design: design,
                    expectedRevision: eyeLibraryRevision
                )
            }
            guard snapshot.ok else {
                await handleLibraryConflict(snapshot, client: client)
                return
            }
            let savedId = snapshot.savedId ?? selectedEyeLibraryEntry?.id
            applyLibrarySnapshot(snapshot, preferredId: savedId)
            statusText = "已写入“\(design.baseMood)”表情槽位（版本 \(snapshot.revision)）；其他表情不受影响"
        } catch {
            statusText = "保存机器人眼形失败：\(error.localizedDescription)"
        }
    }

    func deleteSelectedEye(robotBaseURLString: String) async {
        guard let entry = selectedEyeLibraryEntry, entry.deletable else {
            statusText = "这个表情还没有手机上传的替换眼形"
            return
        }
        guard let client = robotClient(robotBaseURLString: robotBaseURLString) else {
            statusText = "机器人地址无效"
            return
        }
        isBusy = true
        defer { isBusy = false }
        do {
            let snapshot = try await client.deleteEyeLibraryEntry(
                id: entry.id,
                expectedRevision: eyeLibraryRevision
            )
            guard snapshot.ok else {
                await handleLibraryConflict(snapshot, client: client)
                return
            }
            applyLibrarySnapshot(snapshot, preferredId: entry.id)
            statusText = "已删除“\(libraryDisplayName(entry))”的自定义替换，恢复固件原始眼形"
        } catch {
            statusText = "删除机器人眼形失败：\(error.localizedDescription)"
        }
    }

    func activateSelectedEye(robotBaseURLString: String) async {
        guard let entry = selectedEyeLibraryEntry else {
            statusText = "请先保存新眼形，再设为机器人当前眼形"
            return
        }
        guard let client = robotClient(robotBaseURLString: robotBaseURLString) else {
            statusText = "机器人地址无效"
            return
        }
        isBusy = true
        defer { isBusy = false }
        do {
            if isPreviewing {
                clearPreviewTasks()
                isPreviewing = false
                _ = try await client.stopEyeShapePreview()
            }
            let snapshot = try await client.activateEyeLibraryEntry(
                id: entry.id,
                expectedRevision: eyeLibraryRevision
            )
            guard snapshot.ok else {
                await handleLibraryConflict(snapshot, client: client)
                return
            }
            applyLibrarySnapshot(snapshot, preferredId: entry.id)
            statusText = "机器人已切换到“\(libraryDisplayName(entry))”；眨眼、视线和情绪流程继续运行"
        } catch {
            statusText = "切换机器人眼形失败：\(error.localizedDescription)"
        }
    }

    func updatePoint(index: Int, point: NormalizedPoint) {
        guard design.points.indices.contains(index) else { return }
        selectedPointIndex = index
        design.points[index] = point
    }

    func updateHandle(index: Int, incoming: Bool, offset: EyeHandleOffset) {
        guard design.points.indices.contains(index) else { return }
        selectedPointIndex = index
        design.setLinkedHandle(offset, incoming: incoming, at: index)
    }

    func resetSelectedHandles() {
        guard design.points.indices.contains(selectedPointIndex) else { return }
        design.resetHandles(at: selectedPointIndex)
        statusText = "控制点 \(selectedPointIndex + 1) 已恢复自动平滑手柄"
    }

    func selectPoint(_ index: Int) {
        guard design.points.indices.contains(index) else { return }
        selectedPointIndex = index
    }

    func setSmoothingEnabled(_ enabled: Bool) {
        design.smoothingEnabled = enabled
        statusText = enabled
            ? "曲线平滑已开启；可把选中的控制点单独设为尖角"
            : "曲线平滑已关闭；全部控制点按直线连接"
    }

    func setSelectedPointSmooth(_ smooth: Bool) {
        guard design.points.indices.contains(selectedPointIndex) else { return }
        design.setPointSmooth(smooth, at: selectedPointIndex)
        statusText = "控制点 \(selectedPointIndex + 1) 已设为\(smooth ? "平滑点" : "尖角点")"
    }

    func setPreviewBlinkEnabled(_ enabled: Bool, robotBaseURLString: String) {
        previewBlinkEnabled = enabled
        statusText = enabled ? "机器人预览将每隔约 3.2 秒自动眨眼" : "自动眨眼已关闭，仍可手动触发一次"
        schedulePreviewRefresh(robotBaseURLString: robotBaseURLString)
    }

    func triggerPreviewBlink(robotBaseURLString: String) async {
        guard isPreviewing else {
            statusText = "请先开启机器人实时同步"
            return
        }
        guard await pushPreview(
            robotBaseURLString: robotBaseURLString,
            reportsSuccess: false,
            blinkNow: true
        ) else { return }
        statusText = "已让机器人按实际运行路径眨眼一次"
    }

    var selectedPointIsSmooth: Bool {
        design.isPointSmooth(at: selectedPointIndex)
    }

    func updateStyle(_ update: (inout RobotFaceStyle) -> Void) {
        update(&design.style)
        design.style = RobotFaceStyle(
            eyeWidthScale: design.style.eyeWidthScale,
            eyeHeightScale: design.style.eyeHeightScale,
            cornerRadius: design.style.cornerRadius,
            eyeOffsetX: design.style.eyeOffsetX,
            eyeOffsetY: design.style.eyeOffsetY,
            eyeSpacing: design.style.eyeSpacing,
            eyeColor: design.style.eyeColor,
            glowEnabled: design.style.glowEnabled,
            glowStrength: design.style.glowStrength,
            scanlinesEnabled: design.style.scanlinesEnabled,
            scanlineSpacing: design.style.scanlineSpacing,
            scanlineAlpha: design.style.scanlineAlpha
        )
    }

    func setReferenceImage(_ image: UIImage?) {
        referenceImage = image
        statusText = image == nil ? "参考图已清除" : "参考图已载入，点击“参考图生成眼形”"
    }

    func traceReferenceImage() {
        guard let cgImage = referenceImage?.cgImage else {
            statusText = "请先选择参考图片"
            return
        }
        isBusy = true
        defer { isBusy = false }
        do {
            let points = try ReferenceEyeTracer.trace(cgImage: cgImage)
            guard points.count >= 3 else { throw ReferenceEyeTracer.TraceError.noEyeContour }
            design.replacePoints(points, smoothPoints: Array(repeating: true, count: points.count))
            design.smoothingEnabled = true
            design.name = "参考图眼形"
            selectedPointIndex = 0
            statusText = "已从参考图生成 \(points.count) 个控制点，可继续手动微调"
        } catch {
            statusText = "参考生成失败：\(error.localizedDescription)。建议使用黑白、轮廓清楚的眼睛图片。"
        }
    }

    func startPreview(robotBaseURLString: String) async {
        clearPreviewTasks()
        isBusy = true
        defer { isBusy = false }

        guard await pushPreview(robotBaseURLString: robotBaseURLString, reportsSuccess: true) else { return }
        isPreviewing = true
        startPreviewKeepAlive(robotBaseURLString: robotBaseURLString)
    }

    func schedulePreviewRefresh(robotBaseURLString: String) {
        guard isPreviewing else { return }
        previewRefreshPending = true
        guard previewRefreshTask == nil else { return }
        previewRefreshTask = Task { @MainActor [weak self] in
            while let self, self.isPreviewing, !Task.isCancelled {
                guard self.previewRefreshPending else { break }
                if let lastSendAt = self.previewLastSendAt {
                    let minimumInterval = Duration.milliseconds(80)
                    let elapsed = lastSendAt.duration(to: .now)
                    if elapsed < minimumInterval {
                        do {
                            try await Task.sleep(for: minimumInterval - elapsed)
                        } catch {
                            return
                        }
                    }
                }
                guard self.previewRefreshPending else { break }
                self.previewRefreshPending = false
                self.previewLastSendAt = .now
                guard await self.pushPreview(
                    robotBaseURLString: robotBaseURLString,
                    reportsSuccess: false
                ) else { return }
            }
            guard let self, !Task.isCancelled else { return }
            self.previewRefreshTask = nil
            if self.previewRefreshPending && self.isPreviewing {
                self.schedulePreviewRefresh(robotBaseURLString: robotBaseURLString)
            }
        }
    }

    func stopPreview(robotBaseURLString: String) async {
        let wasPreviewing = isPreviewing
        clearPreviewTasks()
        isPreviewing = false
        guard wasPreviewing else { return }

        let normalized = RobotConnectionDefaults.normalizedBaseURL(robotBaseURLString)
        guard let url = URL(string: normalized) else {
            statusText = "机器人地址无效"
            return
        }
        isBusy = true
        defer { isBusy = false }
        do {
            let client = RobotClient(baseURL: url)
            let result = try await client.stopEyeShapePreview()
            statusText = result.ok
                ? "临时预览已停止，机器人已恢复原来的表情运行"
                : "机器人拒绝：\(result.error ?? "未知错误")"
        } catch {
            statusText = "停止命令未送达；机器人会在最多 15 秒后自动恢复：\(error.localizedDescription)"
        }
    }

    private func pushPreview(
        robotBaseURLString: String,
        reportsSuccess: Bool,
        blinkNow: Bool = false
    ) async -> Bool {
        let normalized = RobotConnectionDefaults.normalizedBaseURL(robotBaseURLString)
        guard let url = URL(string: normalized) else {
            previewDidFail(message: "机器人地址无效")
            return false
        }
        do {
            let client = RobotClient(baseURL: url)
            let result = try await client.previewEyeShapeDesign(
                design,
                durationMs: 15_000,
                blinkEnabled: previewBlinkEnabled,
                blinkNow: blinkNow
            )
            guard result.ok else {
                previewDidFail(message: "机器人拒绝临时预览：\(result.error ?? "未知错误")")
                return false
            }
            if reportsSuccess {
                statusText = "机器人正在临时预览；编辑会实时刷新，停止或离开页面即恢复原表情"
            }
            return true
        } catch {
            previewDidFail(message: "临时预览失败：\(error.localizedDescription)。机器人固件需要支持预览接口。")
            return false
        }
    }

    private func startPreviewKeepAlive(robotBaseURLString: String) {
        previewKeepAliveTask?.cancel()
        previewKeepAliveTask = Task { [weak self] in
            while !Task.isCancelled {
                do {
                    try await Task.sleep(for: .seconds(8))
                } catch {
                    return
                }
                guard !Task.isCancelled, let self, self.isPreviewing else { return }
                guard await self.pushPreview(robotBaseURLString: robotBaseURLString, reportsSuccess: false) else {
                    return
                }
            }
        }
    }

    private func previewDidFail(message: String) {
        clearPreviewTasks()
        isPreviewing = false
        statusText = message
    }

    private func clearPreviewTasks() {
        previewRefreshTask?.cancel()
        previewRefreshTask = nil
        previewKeepAliveTask?.cancel()
        previewKeepAliveTask = nil
        previewRefreshPending = false
        previewLastSendAt = nil
    }

    private func robotClient(robotBaseURLString: String) -> RobotClient? {
        let normalized = RobotConnectionDefaults.normalizedBaseURL(robotBaseURLString)
        guard let url = URL(string: normalized) else { return nil }
        return RobotClient(baseURL: url)
    }

    private func applyLibrarySnapshot(_ snapshot: RobotEyeLibrarySnapshot, preferredId: String?) {
        eyeLibraryEntries = snapshot.entries
        eyeLibraryRevision = snapshot.revision
        activeEyeLibraryId = snapshot.activeId
        customEyeCapacity = snapshot.customCapacity
        let candidates = [preferredId, selectedEyeLibraryId, snapshot.activeId]
            .compactMap { $0 }
        selectedEyeLibraryId = candidates.first { candidate in
            snapshot.entries.contains { $0.id == candidate }
        } ?? snapshot.entries.first?.id ?? ""
        isEditingOverrideDraft = false
        if let entry = selectedEyeLibraryEntry {
            loadLibraryEntryIntoEditor(entry)
        }
    }

    private func loadLibraryEntryIntoEditor(_ entry: RobotEyeLibraryEntry) {
        isEditingOverrideDraft = false
        if let storedDesign = entry.design {
            design = storedDesign
        } else {
            design = EyeShapeDesign(
                name: "\(libraryDisplayName(entry))替换眼形",
                baseMood: entry.baseMood,
                points: [],
                smoothingEnabled: false,
                smoothPoints: [],
                controlHandles: [],
                style: entry.style
            )
        }
        selectedPointIndex = min(selectedPointIndex, max(0, design.points.count - 1))
    }

    private func handleLibraryConflict(
        _ response: RobotEyeLibrarySnapshot,
        client: RobotClient
    ) async {
        let wasConflict = response.revision > 0 && response.revision != eyeLibraryRevision
        do {
            let latest = try await client.eyeLibrary()
            if latest.ok { applyLibrarySnapshot(latest, preferredId: selectedEyeLibraryId) }
            statusText = wasConflict
                ? "机器人眼睛库已被另一端修改，已自动同步最新版本；请确认后再保存"
                : "机器人拒绝操作：\(response.error ?? "未知错误")"
        } catch {
            statusText = "机器人拒绝操作：\(response.error ?? "未知错误")"
        }
    }

    private func libraryDisplayName(_ entry: RobotEyeLibraryEntry) -> String {
        Self.moodDisplayNames[entry.baseMood] ?? entry.name
    }

    deinit {
        previewRefreshTask?.cancel()
        previewKeepAliveTask?.cancel()
    }
}

private extension ExpressionStudioModel {
    static let moodDisplayNames = [
        "neutral": "平静", "happy": "开心", "angry": "生气", "sad": "难过",
        "sleeping": "困倦", "tired": "疲惫", "surprised": "惊讶", "love": "喜爱",
        "dizzy": "眩晕", "wink": "眨眼", "dead": "无语", "thinking": "思考",
        "curious": "好奇", "sunglasses": "墨镜", "loading": "加载"
    ]

    static let previewSafetyExplanation =
        "开启后，拖动锚点、曲线手柄和参数会持续同步到机器人。它使用独立临时覆盖层，不改写机器人的基础表情、参数或表情流程；停止同步、离开本页，或通信中断满 15 秒后都会自动恢复。"
}

struct ExpressionStudioView: View {
    @Binding var robotBaseURLString: String
    @StateObject private var model = ExpressionStudioModel()
    @State private var selectedPhoto: PhotosPickerItem?
    @State private var presetCategoryFilter = "all"
    @State private var selectedEmotionPresetId = ""
    @State private var glyphCategoryFilter = "all"
    @State private var selectedGlyphPresetId = ""
    @State private var showAdvancedPathEditor = false
    @State private var exportDocument: EyeShapeDocument?
    @State private var isExporting = false

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 16) {
                RobotFacePreview(design: model.design, referenceImage: model.referenceImage)
                    .frame(maxWidth: .infinity)
                    .aspectRatio(4 / 3, contentMode: .fit)
                    .clipShape(RoundedRectangle(cornerRadius: 16))
                    .overlay(alignment: .bottomLeading) {
                        Text("320 × 240 机器人预览")
                            .font(.caption.monospacedDigit())
                            .foregroundStyle(.white.opacity(0.7))
                            .padding(10)
                    }
                    .overlay(alignment: .topLeading) {
                        if !model.canEditSelectedShape {
                            Text("固件原始程序化眼形请以机器人实机为准")
                                .font(.caption.weight(.semibold))
                                .padding(8)
                                .background(.black.opacity(0.65), in: Capsule())
                                .foregroundStyle(.white)
                                .padding(10)
                        }
                    }

                GroupBox("机器人表情眼形") {
                    VStack(alignment: .leading, spacing: 12) {
                        Picker("选择表情槽位", selection: $model.selectedEyeLibraryId) {
                            ForEach(model.eyeLibraryEntries) { entry in
                                HStack {
                                    Text(moodDisplayName(entry.baseMood))
                                    if entry.hasOverride { Text("· 已替换") }
                                }
                                .tag(entry.id)
                            }
                        }
                        .pickerStyle(.menu)
                        .onChange(of: model.selectedEyeLibraryId) { _, id in
                            model.selectEyeLibraryEntry(id)
                        }

                        TextField("替换眼形名称", text: $model.design.name)
                            .textFieldStyle(.roundedBorder)
                            .disabled(!model.canEditSelectedShape)

                        VStack(alignment: .leading, spacing: 10) {
                            HStack(spacing: 10) {
                                Button {
                                    Task { await model.syncEyeLibrary(robotBaseURLString: robotBaseURLString) }
                                } label: {
                                    Label("同步机器人库", systemImage: "arrow.triangle.2.circlepath")
                                }
                            }

                            HStack(spacing: 10) {
                                Button {
                                    Task { await model.saveSelectedEye(robotBaseURLString: robotBaseURLString) }
                                } label: {
                                    Label(
                                        model.canEditSelectedShape ? "保存替换到固件" : "保存参数",
                                        systemImage: "square.and.arrow.down"
                                    )
                                }

                                Button {
                                    Task { await model.activateSelectedEye(robotBaseURLString: robotBaseURLString) }
                                } label: {
                                    Label("在机器人显示", systemImage: "eyes")
                                }
                            }
                        }
                        .buttonStyle(.bordered)
                        .disabled(model.isBusy || model.eyeLibraryEntries.isEmpty)

                        if model.canDeleteSelectedEye {
                            Button(role: .destructive) {
                                Task { await model.deleteSelectedEye(robotBaseURLString: robotBaseURLString) }
                            } label: {
                                Label("删除替换并恢复固件原眼形", systemImage: "arrow.uturn.backward.circle")
                            }
                            .buttonStyle(.bordered)
                            .disabled(model.isBusy)
                        }

                        HStack {
                            Text("当前槽位：\(moodDisplayName(model.design.baseMood))")
                            Spacer()
                            if let entry = model.selectedEyeLibraryEntry, entry.id == model.activeEyeLibraryId {
                                Label("机器人正在显示", systemImage: "checkmark.circle.fill")
                                    .foregroundStyle(.green)
                            }
                        }
                        .font(.footnote.weight(.semibold))

                        Text("15 个系统表情槽位保持不变。上传只替换所选槽位的眼睛轮廓；情绪、待机、动作、视线和眨眼仍按原逻辑工作。删除只移除手机上传的替换，不删除系统表情。")
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }
                }

                GroupBox("新实心眼型库") {
                    VStack(alignment: .leading, spacing: 12) {
                        HStack {
                            Picker("眼型批次", selection: $presetCategoryFilter) {
                                Text("全部 · \(SolidEyePreset.allCases.count)").tag("all")
                                ForEach(SolidEyePresetCollection.allCases) { collection in
                                    Text("\(collection.title) · \(presetCount(in: collection))").tag(collection.rawValue)
                                }
                            }
                            .pickerStyle(.menu)

                            Spacer()
                            Text("点一下直接选用")
                                .font(.footnote)
                                .foregroundStyle(.secondary)
                        }

                        LazyVGrid(
                            columns: [
                                GridItem(.adaptive(minimum: 132, maximum: 190), spacing: 10)
                            ],
                            spacing: 10
                        ) {
                            ForEach(filteredEmotionPresets) { preset in
                                Button {
                                    selectedEmotionPresetId = preset.id
                                    selectedGlyphPresetId = ""
                                    model.applySolidPreset(preset)
                                } label: {
                                    SolidEyePresetCard(
                                        preset: preset,
                                        color: Color(hex: model.design.style.eyeColor),
                                        isSelected: selectedEmotionPresetId == preset.id
                                    )
                                }
                                .buttonStyle(.plain)
                                .disabled(model.isBusy || model.eyeLibraryEntries.isEmpty)
                            }
                        }

                        Text("旧基础眼型已移除；这里仅保留刚采纳的墨镜、轻松反应和机器人个性三批实心眼型。轮廓已按机器人单眼约 72 × 54 像素优化，默认无需路径编辑。")
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }
                }

                GroupBox("符号与图案眼睛库") {
                    VStack(alignment: .leading, spacing: 12) {
                        HStack {
                            Picker("图案分类", selection: $glyphCategoryFilter) {
                                Text("全部 · \(EyeGlyphPreset.allCases.count)").tag("all")
                                ForEach(EyeGlyphCategory.allCases) { category in
                                    Text(category.title).tag(category.rawValue)
                                }
                            }
                            .pickerStyle(.menu)

                            Spacer()
                            Text("手机与机器人同名 glyph")
                                .font(.footnote)
                                .foregroundStyle(.secondary)
                        }

                        LazyVGrid(
                            columns: [GridItem(.adaptive(minimum: 112, maximum: 168), spacing: 10)],
                            spacing: 10
                        ) {
                            ForEach(filteredGlyphPresets) { preset in
                                Button {
                                    selectedGlyphPresetId = preset.id
                                    selectedEmotionPresetId = ""
                                    model.applyGlyphPreset(preset)
                                } label: {
                                    EyeGlyphPresetCard(
                                        preset: preset,
                                        color: Color(hex: model.design.style.eyeColor),
                                        isSelected: selectedGlyphPresetId == preset.id
                                    )
                                }
                                .buttonStyle(.plain)
                                .disabled(model.isBusy || model.eyeLibraryEntries.isEmpty)
                            }
                        }

                        Text("手机与 Firmware 使用同一份 192×192 图案蒙版；本批还包含指纹、二维码、机器人和交通工具等细线/多区域压力图案，用来验证机器人实际显示的细节边界。")
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }
                }

                GroupBox("眼睛大小与位置") {
                    VStack(alignment: .leading, spacing: 13) {
                        styleSlider("眼睛宽度", value: model.design.style.eyeWidthScale, range: 0.35...2.2, display: formatRatio(model.design.style.eyeWidthScale)) {
                            model, value in model.updateStyle { $0.eyeWidthScale = value }
                        }
                        styleSlider("眼睛高度", value: model.design.style.eyeHeightScale, range: 0.15...2.2, display: formatRatio(model.design.style.eyeHeightScale)) {
                            model, value in model.updateStyle { $0.eyeHeightScale = value }
                        }
                        styleSlider("左右间距", value: Double(model.design.style.eyeSpacing), range: 48...220, display: "\(model.design.style.eyeSpacing)") {
                            model, value in model.updateStyle { $0.eyeSpacing = Int(value.rounded()) }
                        }
                        styleSlider("横向偏移", value: Double(model.design.style.eyeOffsetX), range: -90...90, display: "\(model.design.style.eyeOffsetX)") {
                            model, value in model.updateStyle { $0.eyeOffsetX = Int(value.rounded()) }
                        }
                        styleSlider("纵向偏移", value: Double(model.design.style.eyeOffsetY), range: -90...90, display: "\(model.design.style.eyeOffsetY)") {
                            model, value in model.updateStyle { $0.eyeOffsetY = Int(value.rounded()) }
                        }

                        RobotEyeColorPalette(
                            colorHex: model.design.style.eyeColor,
                            onChange: { hex in model.updateStyle { $0.eyeColor = hex } }
                        )

                        Toggle("发光", isOn: Binding(
                            get: { model.design.style.glowEnabled },
                            set: { enabled in model.updateStyle { $0.glowEnabled = enabled } }
                        ))
                        Toggle("扫描线", isOn: Binding(
                            get: { model.design.style.scanlinesEnabled },
                            set: { enabled in model.updateStyle { $0.scanlinesEnabled = enabled } }
                        ))
                        Toggle("扫光特效", isOn: $model.design.sweepEnabled)
                        Text(model.design.sweepEnabled
                             ? "已开启：手机预览与机器人自定义眼形会循环播放一道高亮扫光。"
                             : "未开启：眼形保持原来的静态实心显示。")
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }
                }

                if !model.design.isGlyph {
                    GroupBox("高级：路径与参考图（可选）") {
                        DisclosureGroup(isExpanded: $showAdvancedPathEditor) {
                            VStack(alignment: .leading, spacing: 10) {
                                EyeShapeEditor(
                                    points: model.design.points,
                                    smoothingEnabled: model.design.smoothingEnabled,
                                    smoothPoints: model.design.smoothPoints,
                                    controlHandles: model.design.controlHandles,
                                    selectedPointIndex: model.selectedPointIndex,
                                    referenceImage: model.referenceImage,
                                    color: Color(hex: model.design.style.eyeColor),
                                    onSelect: model.selectPoint,
                                    onMove: model.updatePoint,
                                    onMoveHandle: model.updateHandle
                                )
                                .frame(height: 220)

                                Toggle(
                                    "曲线平滑模式",
                                    isOn: Binding(
                                        get: { model.design.smoothingEnabled },
                                        set: { enabled in model.setSmoothingEnabled(enabled) }
                                    )
                                )

                                HStack(spacing: 12) {
                                    Text("控制点 \(model.selectedPointIndex + 1)")
                                        .font(.subheadline.monospacedDigit().weight(.semibold))

                                    Picker(
                                        "控制点类型",
                                        selection: Binding(
                                            get: { model.selectedPointIsSmooth },
                                            set: { smooth in model.setSelectedPointSmooth(smooth) }
                                        )
                                    ) {
                                        Label("平滑点", systemImage: "circle.fill").tag(true)
                                        Label("尖角点", systemImage: "diamond.fill").tag(false)
                                    }
                                    .pickerStyle(.segmented)
                                    .disabled(!model.design.smoothingEnabled)

                                    Button {
                                        model.resetSelectedHandles()
                                    } label: {
                                        Label("自动", systemImage: "arrow.counterclockwise")
                                    }
                                    .buttonStyle(.bordered)
                                    .disabled(!model.selectedPointIsSmooth)
                                }

                                Text("选中平滑点后会显示一对联动手柄：拖动任一端即可像路径工具一样调整切线，另一端保持对称。菱形尖角点不显示手柄；“自动”可恢复默认平滑。")
                                    .font(.footnote)
                                    .foregroundStyle(.secondary)

                                HStack {
                                    PhotosPicker(selection: $selectedPhoto, matching: .images) {
                                        Label("选择参考图", systemImage: "photo")
                                    }
                                    Button {
                                        model.traceReferenceImage()
                                    } label: {
                                        Label("参考图生成眼形", systemImage: "wand.and.rays")
                                    }
                                    .disabled(model.referenceImage == nil || model.isBusy)
                                    Button {
                                        model.setReferenceImage(nil)
                                        selectedPhoto = nil
                                    } label: {
                                        Image(systemName: "xmark.circle")
                                    }
                                    .accessibilityLabel("清除参考图")
                                }
                                .buttonStyle(.bordered)
                            }
                            .padding(.top, 10)
                        } label: {
                            VStack(alignment: .leading, spacing: 3) {
                                Text("手动微调控制点")
                                    .font(.subheadline.weight(.semibold))
                                Text("默认关闭；新实心眼型不需要路径编辑也能直接使用")
                                    .font(.caption)
                                    .foregroundStyle(.secondary)
                            }
                        }
                    }
                    .disabled(!model.canEditSelectedShape)
                }

                GroupBox("机器人临时预览") {
                    VStack(alignment: .leading, spacing: 12) {
                        HStack {
                            if model.isPreviewing {
                                Button(role: .destructive) {
                                    Task { await model.stopPreview(robotBaseURLString: robotBaseURLString) }
                                } label: {
                                    Label("停止并恢复原表情", systemImage: "stop.circle.fill")
                                }
                                .buttonStyle(.borderedProminent)
                                .disabled(model.isBusy)
                            } else {
                                Button {
                                    Task { await model.startPreview(robotBaseURLString: robotBaseURLString) }
                                } label: {
                                    Label("开始实时同步", systemImage: "eyes")
                                }
                                .buttonStyle(.borderedProminent)
                                .disabled(model.isBusy || !model.canEditSelectedShape)
                            }

                            if model.isPreviewing {
                                Label("实时同步中", systemImage: "dot.radiowaves.left.and.right")
                                    .font(.footnote.weight(.semibold))
                                    .foregroundStyle(.green)
                            }
                        }

                        HStack(spacing: 12) {
                            Toggle(
                                "自动模拟眨眼",
                                isOn: Binding(
                                    get: { model.previewBlinkEnabled },
                                    set: { enabled in
                                        model.setPreviewBlinkEnabled(
                                            enabled,
                                            robotBaseURLString: robotBaseURLString
                                        )
                                    }
                                )
                            )

                            Button {
                                Task { await model.triggerPreviewBlink(robotBaseURLString: robotBaseURLString) }
                            } label: {
                                Label("眨一下", systemImage: "eye")
                            }
                            .buttonStyle(.bordered)
                            .disabled(!model.isPreviewing || model.isBusy)
                        }

                        Text("自动模式按机器人约 3.2 秒一次的真实节奏运行；“眨一下”可立即检查眼形收合和重新张开的效果。")
                            .font(.footnote)
                            .foregroundStyle(.secondary)

                        Text(ExpressionStudioModel.previewSafetyExplanation)
                            .font(.footnote)
                            .foregroundStyle(.secondary)

                        Button {
                            exportDocument = EyeShapeDocument(design: model.design)
                            isExporting = true
                        } label: {
                            Label("导出眼形 JSON", systemImage: "square.and.arrow.up")
                        }
                        .buttonStyle(.bordered)
                        .disabled(!model.canEditSelectedShape)

                        Text(model.statusText)
                            .font(.footnote)
                            .foregroundStyle(.secondary)
                    }
                }
            }
            .padding()
        }
        .onChange(of: selectedPhoto) { _, item in
            guard let item else { return }
            Task {
                if let data = try? await item.loadTransferable(type: Data.self), let image = UIImage(data: data) {
                    model.setReferenceImage(image)
                }
            }
        }
        .onChange(of: model.design) { _, _ in
            model.schedulePreviewRefresh(robotBaseURLString: robotBaseURLString)
        }
        .onDisappear {
            Task { await model.stopPreview(robotBaseURLString: robotBaseURLString) }
        }
        .task {
            await model.syncEyeLibrary(robotBaseURLString: robotBaseURLString)
        }
        .fileExporter(
            isPresented: $isExporting,
            document: exportDocument,
            contentType: .json,
            defaultFilename: "robot-eye-shape.json"
        ) { _ in }
    }

    private func styleSlider(
        _ title: String,
        value: Double,
        range: ClosedRange<Double>,
        display: String,
        update: @escaping (ExpressionStudioModel, Double) -> Void
    ) -> some View {
        VStack(spacing: 5) {
            HStack {
                Text(title)
                Spacer()
                Text(display).font(.footnote.monospacedDigit()).foregroundStyle(.secondary)
            }
            Slider(value: Binding(get: { value }, set: { update(model, $0) }), in: range)
        }
    }

    private func moodDisplayName(_ mood: String) -> String {
        [
            "neutral": "平静", "happy": "开心", "angry": "生气", "sad": "难过",
            "sleeping": "困倦", "tired": "疲惫", "surprised": "惊讶", "love": "喜爱",
            "dizzy": "眩晕", "wink": "眨眼", "dead": "无语", "thinking": "思考",
            "curious": "好奇", "sunglasses": "墨镜", "loading": "加载"
        ][mood] ?? mood
    }

    private var filteredEmotionPresets: [SolidEyePreset] {
        guard presetCategoryFilter != "all" else { return SolidEyePreset.allCases }
        return SolidEyePreset.allCases.filter { $0.collection.rawValue == presetCategoryFilter }
    }

    private func presetCount(in collection: SolidEyePresetCollection) -> Int {
        SolidEyePreset.allCases.filter { $0.collection == collection }.count
    }

    private var filteredGlyphPresets: [EyeGlyphPreset] {
        guard glyphCategoryFilter != "all" else { return EyeGlyphPreset.allCases }
        return EyeGlyphPreset.allCases.filter { $0.category.rawValue == glyphCategoryFilter }
    }

    private func formatRatio(_ value: Double) -> String { String(format: "%.2fx", value) }
}

private struct SolidEyePresetCard: View {
    let preset: SolidEyePreset
    let color: Color
    let isSelected: Bool

    var body: some View {
        VStack(alignment: .leading, spacing: 7) {
            ZStack {
                RoundedRectangle(cornerRadius: 10)
                    .fill(Color.black)

                HStack(spacing: 0) {
                    ApprovedEyePairSymbol(
                        presetId: preset.glyphId,
                        side: .left,
                        color: color
                    )
                    ApprovedEyePairSymbol(
                        presetId: preset.glyphId,
                        side: .right,
                        color: color
                    )
                }
                .frame(width: previewWidth * 2, height: previewHeight)
                .shadow(color: color.opacity(0.35), radius: 3)
            }
            .frame(height: 62)

            Text(preset.title)
                .font(.subheadline.weight(.semibold))
                .foregroundStyle(.primary)
                .lineLimit(1)

            Text(preset.emotionHint)
                .font(.caption2)
                .foregroundStyle(.secondary)
                .lineLimit(1)
        }
        .padding(9)
        .background(
            RoundedRectangle(cornerRadius: 13)
                .fill(isSelected ? Color.accentColor.opacity(0.12) : Color.secondary.opacity(0.08))
        )
        .overlay {
            RoundedRectangle(cornerRadius: 13)
                .stroke(isSelected ? Color.accentColor : Color.secondary.opacity(0.20), lineWidth: isSelected ? 2 : 1)
        }
        .contentShape(RoundedRectangle(cornerRadius: 13))
        .accessibilityElement(children: .combine)
        .accessibilityLabel("\(preset.title)，\(preset.emotionHint)")
        .accessibilityHint("载入到当前机器人表情槽位")
    }

    private var previewWidth: CGFloat {
        44
    }

    private var previewHeight: CGFloat {
        56
    }
}

private struct EyeGlyphPresetCard: View {
    let preset: EyeGlyphPreset
    let color: Color
    let isSelected: Bool

    var body: some View {
        VStack(alignment: .leading, spacing: 7) {
            ZStack {
                RoundedRectangle(cornerRadius: 10).fill(.black)
                EyeGlyphSymbol(glyphId: preset.rawValue, color: color)
                    .frame(width: 48, height: 40)
                    .shadow(color: color.opacity(0.35), radius: 3)
            }
            .frame(height: 58)

            Text(preset.title)
                .font(.subheadline.weight(.semibold))
                .foregroundStyle(.primary)
                .lineLimit(1)
        }
        .padding(9)
        .background(
            RoundedRectangle(cornerRadius: 13)
                .fill(isSelected ? Color.accentColor.opacity(0.12) : Color.secondary.opacity(0.08))
        )
        .overlay {
            RoundedRectangle(cornerRadius: 13)
                .stroke(isSelected ? Color.accentColor : Color.secondary.opacity(0.20), lineWidth: isSelected ? 2 : 1)
        }
        .contentShape(RoundedRectangle(cornerRadius: 13))
        .accessibilityElement(children: .combine)
        .accessibilityLabel("\(preset.category.title)，\(preset.title)")
        .accessibilityHint("载入到当前机器人表情槽位")
    }
}

private struct EyeGlyphSymbol: View {
    let glyphId: String
    let color: Color

    var body: some View {
        if let image = EyeGlyphImageCatalog.images[glyphId] {
            Image(uiImage: image)
                .resizable()
                .interpolation(.high)
                .renderingMode(.template)
                .foregroundStyle(color)
        } else {
            Image(systemName: "questionmark")
                .resizable()
                .scaledToFit()
                .foregroundStyle(color)
        }
    }
}

@MainActor
private enum EyeGlyphImageCatalog {
    static let images: [String: UIImage] = Dictionary(
        uniqueKeysWithValues: EyeGlyphBitmapCatalog.ids.compactMap { id in
            guard let image = makeImage(id: id) else { return nil }
            return (id, image)
        }
    )

    private static func makeImage(id: String) -> UIImage? {
        guard let bitmap = EyeGlyphBitmapCatalog.bitmap(for: id) else { return nil }
        let width = EyeGlyphBitmapCatalog.width
        let height = EyeGlyphBitmapCatalog.height
        var rgba = [UInt8](repeating: 0, count: width * height * 4)
        for y in 0..<height {
            for x in 0..<width where EyeGlyphBitmapCatalog.pixelIsSet(in: bitmap, x: x, y: y) {
                let offset = (y * width + x) * 4
                rgba[offset] = 255
                rgba[offset + 1] = 255
                rgba[offset + 2] = 255
                rgba[offset + 3] = 255
            }
        }
        guard let provider = CGDataProvider(data: Data(rgba) as CFData),
              let colorSpace = CGColorSpace(name: CGColorSpace.sRGB),
              let cgImage = CGImage(
                width: width,
                height: height,
                bitsPerComponent: 8,
                bitsPerPixel: 32,
                bytesPerRow: width * 4,
                space: colorSpace,
                bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedLast.rawValue),
                provider: provider,
                decode: nil,
                shouldInterpolate: true,
                intent: .defaultIntent
              ) else { return nil }
        return UIImage(cgImage: cgImage, scale: 1, orientation: .up)
            .withRenderingMode(.alwaysTemplate)
    }
}

private struct ApprovedEyePairSymbol: View {
    let presetId: String
    let side: ApprovedEyePairBitmapCatalog.Side
    let color: Color

    var body: some View {
        if let image = ApprovedEyePairImageCatalog.image(for: presetId, side: side) {
            Image(uiImage: image)
                .resizable()
                .interpolation(.high)
                .renderingMode(.template)
                .foregroundStyle(color)
        } else {
            Color.clear
        }
    }
}

@MainActor
private enum ApprovedEyePairImageCatalog {
    private static let leftImages = makeImages(side: .left)
    private static let rightImages = makeImages(side: .right)

    static func image(
        for id: String,
        side: ApprovedEyePairBitmapCatalog.Side
    ) -> UIImage? {
        side == .left ? leftImages[id] : rightImages[id]
    }

    private static func makeImages(
        side: ApprovedEyePairBitmapCatalog.Side
    ) -> [String: UIImage] {
        Dictionary(
            uniqueKeysWithValues: ApprovedEyePairBitmapCatalog.ids.compactMap { id in
                guard let image = makeImage(id: id, side: side) else { return nil }
                return (id, image)
            }
        )
    }

    private static func makeImage(
        id: String,
        side: ApprovedEyePairBitmapCatalog.Side
    ) -> UIImage? {
        guard let bitmap = ApprovedEyePairBitmapCatalog.bitmap(for: id, side: side) else {
            return nil
        }
        let width = ApprovedEyePairBitmapCatalog.width
        let height = ApprovedEyePairBitmapCatalog.height
        var rgba = [UInt8](repeating: 0, count: width * height * 4)
        for y in 0..<height {
            for x in 0..<width {
                let alpha = ApprovedEyePairBitmapCatalog.coverage(
                    in: bitmap,
                    x: x,
                    y: y
                ) * 17
                guard alpha > 0 else { continue }
                let offset = (y * width + x) * 4
                // CGImageAlphaInfo.premultipliedLast requires RGB to be
                // premultiplied by alpha. The template tint then preserves
                // exactly the same 4-bit edge coverage as firmware.
                rgba[offset] = alpha
                rgba[offset + 1] = alpha
                rgba[offset + 2] = alpha
                rgba[offset + 3] = alpha
            }
        }
        guard let provider = CGDataProvider(data: Data(rgba) as CFData),
              let colorSpace = CGColorSpace(name: CGColorSpace.sRGB),
              let cgImage = CGImage(
                width: width,
                height: height,
                bitsPerComponent: 8,
                bitsPerPixel: 32,
                bytesPerRow: width * 4,
                space: colorSpace,
                bitmapInfo: CGBitmapInfo(rawValue: CGImageAlphaInfo.premultipliedLast.rawValue),
                provider: provider,
                decode: nil,
                shouldInterpolate: true,
                intent: .defaultIntent
              ) else { return nil }
        return UIImage(cgImage: cgImage, scale: 1, orientation: .up)
            .withRenderingMode(.alwaysTemplate)
    }
}

private struct RobotFacePreview: View {
    let design: EyeShapeDesign
    let referenceImage: UIImage?

    var body: some View {
        TimelineView(.animation(minimumInterval: 1.0 / 30.0, paused: !design.sweepEnabled)) { timeline in
            GeometryReader { geometry in
                let sx = geometry.size.width / 320
                let sy = geometry.size.height / 240
                let eyeWidth = CGFloat(72 * design.style.eyeWidthScale) * sx
                let eyeHeight = CGFloat(54 * design.style.eyeHeightScale) * sy
                let centerX = CGFloat(160 + design.style.eyeOffsetX) * sx
                let centerY = CGFloat(120 + design.style.eyeOffsetY) * sy
                let spacing = CGFloat(design.style.eyeSpacing) * sx
                let sweepPhase = timeline.date.timeIntervalSinceReferenceDate
                    .truncatingRemainder(dividingBy: 1.4) / 1.4
                ZStack {
                    Color.black
                    if let referenceImage {
                        Image(uiImage: referenceImage)
                            .resizable()
                            .scaledToFit()
                            .opacity(0.18)
                    }
                    if let glyphId = design.glyphId {
                        if ApprovedEyePairBitmapCatalog.contains(glyphId) {
                            ApprovedEyePairSymbol(
                                presetId: glyphId,
                                side: .left,
                                color: Color(hex: design.style.eyeColor)
                            )
                                .shadow(
                                    color: design.style.glowEnabled ? Color(hex: design.style.eyeColor).opacity(0.8) : .clear,
                                    radius: CGFloat(design.style.glowStrength * 2)
                                )
                                .frame(width: eyeWidth, height: eyeHeight)
                                .position(x: centerX - spacing / 2, y: centerY)
                            ApprovedEyePairSymbol(
                                presetId: glyphId,
                                side: .right,
                                color: Color(hex: design.style.eyeColor)
                            )
                                .shadow(
                                    color: design.style.glowEnabled ? Color(hex: design.style.eyeColor).opacity(0.8) : .clear,
                                    radius: CGFloat(design.style.glowStrength * 2)
                                )
                                .frame(width: eyeWidth, height: eyeHeight)
                                .position(x: centerX + spacing / 2, y: centerY)
                            if design.sweepEnabled {
                                ApprovedEyePairSweepHighlight(
                                    presetId: glyphId,
                                    side: .left,
                                    phase: sweepPhase
                                )
                                    .frame(width: eyeWidth, height: eyeHeight)
                                    .position(x: centerX - spacing / 2, y: centerY)
                                ApprovedEyePairSweepHighlight(
                                    presetId: glyphId,
                                    side: .right,
                                    phase: sweepPhase
                                )
                                    .frame(width: eyeWidth, height: eyeHeight)
                                    .position(x: centerX + spacing / 2, y: centerY)
                            }
                        } else {
                            EyeGlyphSymbol(glyphId: glyphId, color: Color(hex: design.style.eyeColor))
                                .shadow(
                                    color: design.style.glowEnabled ? Color(hex: design.style.eyeColor).opacity(0.8) : .clear,
                                    radius: CGFloat(design.style.glowStrength * 2)
                                )
                                .frame(width: eyeWidth, height: eyeHeight)
                                .position(x: centerX - spacing / 2, y: centerY)
                            EyeGlyphSymbol(glyphId: glyphId, color: Color(hex: design.style.eyeColor))
                                .shadow(
                                    color: design.style.glowEnabled ? Color(hex: design.style.eyeColor).opacity(0.8) : .clear,
                                    radius: CGFloat(design.style.glowStrength * 2)
                                )
                                .frame(width: eyeWidth, height: eyeHeight)
                                .position(x: centerX + spacing / 2, y: centerY)
                            if design.sweepEnabled {
                                EyeGlyphSweepHighlight(glyphId: glyphId, phase: sweepPhase)
                                    .frame(width: eyeWidth, height: eyeHeight)
                                    .position(x: centerX - spacing / 2, y: centerY)
                                EyeGlyphSweepHighlight(glyphId: glyphId, phase: sweepPhase)
                                    .frame(width: eyeWidth, height: eyeHeight)
                                    .position(x: centerX + spacing / 2, y: centerY)
                            }
                        }
                    } else {
                        EyeShapePath(
                            points: design.points,
                            smoothingEnabled: design.smoothingEnabled,
                            smoothPoints: design.smoothPoints,
                            controlHandles: design.controlHandles
                        )
                            .fill(Color(hex: design.style.eyeColor))
                            .shadow(
                                color: design.style.glowEnabled ? Color(hex: design.style.eyeColor).opacity(0.8) : .clear,
                                radius: CGFloat(design.style.glowStrength * 2)
                            )
                            .frame(width: eyeWidth, height: eyeHeight)
                            .position(x: centerX - spacing / 2, y: centerY)
                        EyeShapePath(
                            points: design.points,
                            smoothingEnabled: design.smoothingEnabled,
                            smoothPoints: design.smoothPoints,
                            controlHandles: design.controlHandles,
                            mirrorHorizontally: design.mirrorRightEye
                        )
                            .fill(Color(hex: design.style.eyeColor))
                            .shadow(
                                color: design.style.glowEnabled ? Color(hex: design.style.eyeColor).opacity(0.8) : .clear,
                                radius: CGFloat(design.style.glowStrength * 2)
                            )
                            .frame(width: eyeWidth, height: eyeHeight)
                            .position(x: centerX + spacing / 2, y: centerY)
                        if design.sweepEnabled {
                            EyeShapeSweepHighlight(design: design, mirrorHorizontally: false, phase: sweepPhase)
                                .frame(width: eyeWidth, height: eyeHeight)
                                .position(x: centerX - spacing / 2, y: centerY)
                            EyeShapeSweepHighlight(
                                design: design,
                                mirrorHorizontally: design.mirrorRightEye,
                                phase: sweepPhase
                            )
                                .frame(width: eyeWidth, height: eyeHeight)
                                .position(x: centerX + spacing / 2, y: centerY)
                        }
                    }
                    if design.style.scanlinesEnabled {
                        ScanlineOverlay(spacing: max(3, design.style.scanlineSpacing), alpha: design.style.scanlineAlpha)
                    }
                }
            }
        }
    }
}

private struct EyeShapeSweepHighlight: View {
    let design: EyeShapeDesign
    let mirrorHorizontally: Bool
    let phase: Double

    var body: some View {
        GeometryReader { geometry in
            movingSweep(in: geometry.size)
                .mask(
                    EyeShapePath(
                        points: design.points,
                        smoothingEnabled: design.smoothingEnabled,
                        smoothPoints: design.smoothPoints,
                        controlHandles: design.controlHandles,
                        mirrorHorizontally: mirrorHorizontally
                    )
                    .fill(.white)
                )
        }
    }

    private func movingSweep(in size: CGSize) -> some View {
        let travel = size.width * 1.8
        let x = CGFloat(phase) * travel - size.width * 0.9
        return Rectangle()
            .fill(
                LinearGradient(
                    colors: [.clear, .white.opacity(0.92), .clear],
                    startPoint: .leading,
                    endPoint: .trailing
                )
            )
            .frame(width: max(12, size.width * 0.34), height: size.height * 1.5)
            .rotationEffect(.degrees(-12))
            .offset(x: x)
    }
}

private struct ApprovedEyePairSweepHighlight: View {
    let presetId: String
    let side: ApprovedEyePairBitmapCatalog.Side
    let phase: Double

    var body: some View {
        GeometryReader { geometry in
            let travel = geometry.size.width * 1.8
            let x = CGFloat(phase) * travel - geometry.size.width * 0.9
            Rectangle()
                .fill(
                    LinearGradient(
                        colors: [.clear, .white.opacity(0.92), .clear],
                        startPoint: .leading,
                        endPoint: .trailing
                    )
                )
                .frame(width: max(12, geometry.size.width * 0.34), height: geometry.size.height * 1.5)
                .rotationEffect(.degrees(-12))
                .offset(x: x)
                .mask(ApprovedEyePairSymbol(presetId: presetId, side: side, color: .white))
        }
    }
}

private struct EyeGlyphSweepHighlight: View {
    let glyphId: String
    let phase: Double

    var body: some View {
        GeometryReader { geometry in
            let travel = geometry.size.width * 1.8
            let x = CGFloat(phase) * travel - geometry.size.width * 0.9
            Rectangle()
                .fill(
                    LinearGradient(
                        colors: [.clear, .white.opacity(0.92), .clear],
                        startPoint: .leading,
                        endPoint: .trailing
                    )
                )
                .frame(width: max(12, geometry.size.width * 0.34), height: geometry.size.height * 1.5)
                .rotationEffect(.degrees(-12))
                .offset(x: x)
                .mask(EyeGlyphSymbol(glyphId: glyphId, color: .white))
        }
    }
}

private struct EyeShapeEditor: View {
    let points: [NormalizedPoint]
    let smoothingEnabled: Bool
    let smoothPoints: [Bool]
    let controlHandles: [EyeCurveHandles?]
    let selectedPointIndex: Int
    let referenceImage: UIImage?
    let color: Color
    let onSelect: (Int) -> Void
    let onMove: (Int, NormalizedPoint) -> Void
    let onMoveHandle: (Int, Bool, EyeHandleOffset) -> Void

    var body: some View {
        GeometryReader { geometry in
            ZStack {
                RoundedRectangle(cornerRadius: 12).fill(.black)
                if let referenceImage {
                    Image(uiImage: referenceImage)
                        .resizable()
                        .scaledToFit()
                        .opacity(0.35)
                        .clipShape(RoundedRectangle(cornerRadius: 12))
                }
                EyeShapePath(
                    points: points,
                    smoothingEnabled: smoothingEnabled,
                    smoothPoints: smoothPoints,
                    controlHandles: controlHandles
                )
                    .fill(color.opacity(0.72))
                    .overlay(
                        EyeShapePath(
                            points: points,
                            smoothingEnabled: smoothingEnabled,
                            smoothPoints: smoothPoints,
                            controlHandles: controlHandles
                        )
                        .stroke(.white.opacity(0.85), lineWidth: 2)
                    )
                    .padding(20)

                if let handles = effectiveHandles(at: selectedPointIndex) {
                    handleControls(
                        index: selectedPointIndex,
                        handles: handles,
                        size: geometry.size
                    )
                }

                ForEach(Array(points.indices), id: \.self) { index in
                    pointMarker(index: index)
                        .frame(width: 24, height: 24)
                        .position(
                            x: 20 + CGFloat(points[index].x) * max(1, geometry.size.width - 40),
                            y: 20 + CGFloat(points[index].y) * max(1, geometry.size.height - 40)
                        )
                        .gesture(
                            DragGesture(minimumDistance: 0, coordinateSpace: .named("eye-shape-editor")).onChanged { value in
                                onSelect(index)
                                let x = Double((value.location.x - 20) / max(1, geometry.size.width - 40))
                                let y = Double((value.location.y - 20) / max(1, geometry.size.height - 40))
                                onMove(index, NormalizedPoint(x: x, y: y))
                            }
                        )
                }
            }
            .coordinateSpace(name: "eye-shape-editor")
        }
    }

    @ViewBuilder
    private func handleControls(index: Int, handles: EyeCurveHandles, size: CGSize) -> some View {
        let anchor = editorPoint(points[index], size: size)
        let incoming = editorPoint(points[index], offset: handles.incoming, size: size)
        let outgoing = editorPoint(points[index], offset: handles.outgoing, size: size)

        Path { path in
            path.move(to: incoming)
            path.addLine(to: outgoing)
        }
        .stroke(.orange.opacity(0.9), style: StrokeStyle(lineWidth: 2, dash: [4, 3]))
        .allowsHitTesting(false)

        curveHandle(index: index, incoming: true, position: incoming, anchor: anchor, size: size)
        curveHandle(index: index, incoming: false, position: outgoing, anchor: anchor, size: size)
    }

    private func curveHandle(
        index: Int,
        incoming: Bool,
        position: CGPoint,
        anchor: CGPoint,
        size: CGSize
    ) -> some View {
        Circle()
            .fill(.orange)
            .stroke(.white, lineWidth: 2)
            .frame(width: 18, height: 18)
            .position(position)
            .contentShape(Circle().inset(by: -8))
            .gesture(
                DragGesture(minimumDistance: 0, coordinateSpace: .named("eye-shape-editor"))
                    .onChanged { value in
                        onSelect(index)
                        let width = max(1, size.width - 40)
                        let height = max(1, size.height - 40)
                        onMoveHandle(
                            index,
                            incoming,
                            EyeHandleOffset(
                                x: Double((value.location.x - anchor.x) / width),
                                y: Double((value.location.y - anchor.y) / height)
                            )
                        )
                    }
            )
            .accessibilityLabel(incoming ? "入射曲线手柄" : "出射曲线手柄")
    }

    private func effectiveHandles(at index: Int) -> EyeCurveHandles? {
        guard smoothingEnabled,
              points.indices.contains(index),
              smoothPoints.indices.contains(index),
              smoothPoints[index],
              points.count >= 2 else { return nil }
        if controlHandles.indices.contains(index), let handles = controlHandles[index] {
            return handles
        }
        let previous = points[(index + points.count - 1) % points.count]
        let next = points[(index + 1) % points.count]
        let outgoing = EyeHandleOffset(
            x: (next.x - previous.x) / 6,
            y: (next.y - previous.y) / 6
        )
        return EyeCurveHandles(incoming: -outgoing, outgoing: outgoing)
    }

    private func editorPoint(_ anchor: NormalizedPoint, offset: EyeHandleOffset? = nil, size: CGSize) -> CGPoint {
        CGPoint(
            x: 20 + CGFloat(anchor.x + (offset?.x ?? 0)) * max(1, size.width - 40),
            y: 20 + CGFloat(anchor.y + (offset?.y ?? 0)) * max(1, size.height - 40)
        )
    }

    @ViewBuilder
    private func pointMarker(index: Int) -> some View {
        let isSmooth = smoothingEnabled && smoothPoints.indices.contains(index) && smoothPoints[index]
        let strokeColor: Color = index == selectedPointIndex ? .orange : color
        if isSmooth {
            Circle()
                .fill(.white)
                .stroke(strokeColor, lineWidth: index == selectedPointIndex ? 4 : 3)
        } else {
            RoundedRectangle(cornerRadius: 2)
                .fill(.white)
                .stroke(strokeColor, lineWidth: index == selectedPointIndex ? 4 : 3)
                .rotationEffect(.degrees(45))
                .padding(3)
        }
    }
}

private struct EyeShapePath: Shape {
    let points: [NormalizedPoint]
    let smoothingEnabled: Bool
    let smoothPoints: [Bool]
    let controlHandles: [EyeCurveHandles?]
    var mirrorHorizontally = false

    func path(in rect: CGRect) -> Path {
        let mapped = points.map { point in
            CGPoint(
                x: rect.minX + CGFloat(mirrorHorizontally ? 1 - point.x : point.x) * rect.width,
                y: rect.minY + CGFloat(point.y) * rect.height
            )
        }
        guard let first = mapped.first else { return Path() }
        var path = Path()
        path.move(to: first)
        for index in mapped.indices {
            let nextIndex = (index + 1) % mapped.count
            let current = mapped[index]
            let next = mapped[nextIndex]
            let currentSmooth = isSmooth(index)
            let nextSmooth = isSmooth(nextIndex)
            guard smoothingEnabled, currentSmooth || nextSmooth else {
                path.addLine(to: next)
                continue
            }

            let previous = mapped[(index + mapped.count - 1) % mapped.count]
            let afterNext = mapped[(nextIndex + 1) % mapped.count]
            let control1 = currentSmooth
                ? customControlPoint(anchor: current, index: index, incoming: false, rect: rect)
                    ?? CGPoint(
                        x: current.x + (next.x - previous.x) / 6,
                        y: current.y + (next.y - previous.y) / 6
                    )
                : CGPoint(
                    x: current.x + (next.x - current.x) / 3,
                    y: current.y + (next.y - current.y) / 3
                )
            let control2 = nextSmooth
                ? customControlPoint(anchor: next, index: nextIndex, incoming: true, rect: rect)
                    ?? CGPoint(
                        x: next.x - (afterNext.x - current.x) / 6,
                        y: next.y - (afterNext.y - current.y) / 6
                    )
                : CGPoint(
                    x: next.x - (next.x - current.x) / 3,
                    y: next.y - (next.y - current.y) / 3
                )
            path.addCurve(to: next, control1: control1, control2: control2)
        }
        path.closeSubpath()
        return path
    }

    private func isSmooth(_ index: Int) -> Bool {
        smoothPoints.indices.contains(index) && smoothPoints[index]
    }

    private func customControlPoint(
        anchor: CGPoint,
        index: Int,
        incoming: Bool,
        rect: CGRect
    ) -> CGPoint? {
        guard controlHandles.indices.contains(index), let handles = controlHandles[index] else { return nil }
        let offset = incoming ? handles.incoming : handles.outgoing
        let direction: CGFloat = mirrorHorizontally ? -1 : 1
        return CGPoint(
            x: anchor.x + direction * CGFloat(offset.x) * rect.width,
            y: anchor.y + CGFloat(offset.y) * rect.height
        )
    }
}

private struct RobotEyeColorPalette: View {
    let colorHex: String
    let onChange: (String) -> Void

    private let swatches = [
        "#00F2FF", "#64D2FF", "#0A84FF", "#5E5CE6",
        "#BF5AF2", "#FF2D95", "#FF3B30", "#FF9F0A",
        "#FFE600", "#39FF14", "#00FF62", "#FFFFFF"
    ]

    var body: some View {
        let hsv = hsvComponents
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("眼睛颜色")
                    .font(.subheadline.weight(.semibold))
                Spacer()
                Circle()
                    .fill(Color(hex: colorHex))
                    .frame(width: 28, height: 28)
                    .overlay(Circle().stroke(.primary.opacity(0.35), lineWidth: 1))
                Text(colorHex.uppercased())
                    .font(.caption.monospacedDigit())
                    .foregroundStyle(.secondary)
            }

            LazyVGrid(columns: [GridItem(.adaptive(minimum: 38), spacing: 10)], spacing: 10) {
                ForEach(swatches, id: \.self) { hex in
                    Button {
                        onChange(hex)
                    } label: {
                        Circle()
                            .fill(Color(hex: hex))
                            .frame(width: 34, height: 34)
                            .overlay(
                                Circle().stroke(
                                    hex.caseInsensitiveCompare(colorHex) == .orderedSame ? .orange : .white.opacity(0.35),
                                    lineWidth: hex.caseInsensitiveCompare(colorHex) == .orderedSame ? 4 : 1
                                )
                            )
                            .shadow(color: Color(hex: hex).opacity(0.45), radius: 4)
                    }
                    .buttonStyle(.plain)
                    .accessibilityLabel("颜色 \(hex)")
                }
            }

            PaletteChannelSlider(
                title: "色相",
                value: hsv.hue,
                gradient: Gradient(colors: stride(from: 0.0, through: 1.0, by: 0.1).map {
                    Color(hue: $0, saturation: 1, brightness: 1)
                }),
                onChange: { updateColor(hue: $0, saturation: hsv.saturation, brightness: hsv.brightness) }
            )
            PaletteChannelSlider(
                title: "饱和度",
                value: hsv.saturation,
                gradient: Gradient(colors: [
                    Color(hue: hsv.hue, saturation: 0, brightness: max(0.35, hsv.brightness)),
                    Color(hue: hsv.hue, saturation: 1, brightness: max(0.35, hsv.brightness))
                ]),
                onChange: { updateColor(hue: hsv.hue, saturation: $0, brightness: hsv.brightness) }
            )
            PaletteChannelSlider(
                title: "亮度",
                value: hsv.brightness,
                gradient: Gradient(colors: [
                    .black,
                    Color(hue: hsv.hue, saturation: hsv.saturation, brightness: 1)
                ]),
                onChange: { updateColor(hue: hsv.hue, saturation: hsv.saturation, brightness: $0) }
            )

            HexColorField(colorHex: colorHex, onChange: onChange)
        }
        .padding(.vertical, 4)
    }

    private var hsvComponents: (hue: Double, saturation: Double, brightness: Double) {
        let uiColor = UIColor(Color(hex: colorHex))
        var hue: CGFloat = 0
        var saturation: CGFloat = 0
        var brightness: CGFloat = 0
        var alpha: CGFloat = 0
        guard uiColor.getHue(&hue, saturation: &saturation, brightness: &brightness, alpha: &alpha) else {
            return (0.5, 1, 1)
        }
        return (Double(hue), Double(saturation), Double(brightness))
    }

    private func updateColor(hue: Double, saturation: Double, brightness: Double) {
        let color = UIColor(
            hue: CGFloat(min(max(hue, 0), 1)),
            saturation: CGFloat(min(max(saturation, 0), 1)),
            brightness: CGFloat(min(max(brightness, 0), 1)),
            alpha: 1
        )
        var red: CGFloat = 0
        var green: CGFloat = 0
        var blue: CGFloat = 0
        var alpha: CGFloat = 0
        guard color.getRed(&red, green: &green, blue: &blue, alpha: &alpha) else { return }
        onChange(String(
            format: "#%02X%02X%02X",
            Int((red * 255).rounded()),
            Int((green * 255).rounded()),
            Int((blue * 255).rounded())
        ))
    }
}

private struct PaletteChannelSlider: View {
    let title: String
    let value: Double
    let gradient: Gradient
    let onChange: (Double) -> Void

    var body: some View {
        VStack(spacing: 4) {
            HStack {
                Text(title).font(.caption)
                Spacer()
                Text("\(Int((value * 100).rounded()))%")
                    .font(.caption.monospacedDigit())
                    .foregroundStyle(.secondary)
            }
            GeometryReader { geometry in
                let width = max(1, geometry.size.width)
                ZStack(alignment: .leading) {
                    Capsule()
                        .fill(LinearGradient(gradient: gradient, startPoint: .leading, endPoint: .trailing))
                        .frame(height: 14)
                    Circle()
                        .fill(.white)
                        .stroke(.black.opacity(0.65), lineWidth: 2)
                        .frame(width: 22, height: 22)
                        .shadow(radius: 1)
                        .position(x: CGFloat(value) * width, y: 13)
                }
                .contentShape(Rectangle())
                .gesture(
                    DragGesture(minimumDistance: 0).onChanged { gesture in
                        onChange(Double(min(max(gesture.location.x / width, 0), 1)))
                    }
                )
            }
            .frame(height: 26)
        }
    }
}

private struct HexColorField: View {
    let colorHex: String
    let onChange: (String) -> Void
    @State private var draft = ""

    var body: some View {
        HStack {
            TextField("#00F2FF", text: $draft)
                .textFieldStyle(.roundedBorder)
                .textInputAutocapitalization(.characters)
                .autocorrectionDisabled()
                .font(.body.monospacedDigit())
                .onSubmit(commit)
            Button("应用") { commit() }
                .buttonStyle(.bordered)
        }
        .onAppear { draft = colorHex.uppercased() }
        .onChange(of: colorHex) { _, next in draft = next.uppercased() }
    }

    private func commit() {
        let cleaned = draft.trimmingCharacters(in: CharacterSet.alphanumerics.inverted)
        guard cleaned.count == 6, UInt64(cleaned, radix: 16) != nil else {
            draft = colorHex.uppercased()
            return
        }
        let normalized = "#\(cleaned.uppercased())"
        draft = normalized
        onChange(normalized)
    }
}

private struct ScanlineOverlay: View {
    let spacing: Int
    let alpha: Int

    var body: some View {
        GeometryReader { geometry in
            Path { path in
                for y in stride(from: 0, through: Int(geometry.size.height), by: spacing) {
                    path.move(to: CGPoint(x: 0, y: CGFloat(y)))
                    path.addLine(to: CGPoint(x: geometry.size.width, y: CGFloat(y)))
                }
            }
            .stroke(.black.opacity(Double(alpha) / 100), lineWidth: 1)
        }
        .allowsHitTesting(false)
    }
}

private enum ReferenceEyeTracer {
    enum TraceError: LocalizedError {
        case noEyeContour
        var errorDescription: String? { "没有找到合适的闭合眼形轮廓" }
    }

    static func trace(cgImage: CGImage) throws -> [NormalizedPoint] {
        let request = VNDetectContoursRequest()
        request.contrastAdjustment = 2
        request.detectsDarkOnLight = true
        request.maximumImageDimension = 512
        try VNImageRequestHandler(cgImage: cgImage, options: [:]).perform([request])
        guard let observation = request.results?.first else { throw TraceError.noEyeContour }

        let candidates = observation.topLevelContours.compactMap { contour -> Candidate? in
            var points = [CGPoint]()
            contour.normalizedPath.applyWithBlock { elementPointer in
                let element = elementPointer.pointee
                switch element.type {
                case .moveToPoint, .addLineToPoint:
                    points.append(element.points[0])
                case .addQuadCurveToPoint:
                    points.append(element.points[1])
                case .addCurveToPoint:
                    points.append(element.points[2])
                case .closeSubpath:
                    break
                @unknown default:
                    break
                }
            }
            guard points.count >= 4 else { return nil }
            let xs = points.map(\.x)
            let ys = points.map(\.y)
            guard let minX = xs.min(), let maxX = xs.max(), let minY = ys.min(), let maxY = ys.max() else { return nil }
            let width = maxX - minX
            let height = maxY - minY
            let area = width * height
            guard width > 0.04, height > 0.025, area > 0.003, area < 0.72 else { return nil }
            let aspect = width / max(height, 0.001)
            guard aspect > 0.45, aspect < 8 else { return nil }
            let normalized = points.map { point in
                NormalizedPoint(
                    x: Double((point.x - minX) / width),
                    y: Double(1 - (point.y - minY) / height)
                )
            }
            let score = Double(area) * (aspect >= 1 && aspect <= 4 ? 2 : 1)
            return Candidate(points: simplify(normalized), score: score)
        }
        guard let best = candidates.max(by: { $0.score < $1.score }) else { throw TraceError.noEyeContour }
        return best.points
    }

    private static func simplify(_ points: [NormalizedPoint], maximum: Int = 16) -> [NormalizedPoint] {
        guard points.count > maximum else { return removeNearDuplicates(points) }
        return removeNearDuplicates((0..<maximum).map { points[$0 * points.count / maximum] })
    }

    private static func removeNearDuplicates(_ points: [NormalizedPoint]) -> [NormalizedPoint] {
        var result = [NormalizedPoint]()
        for point in points {
            guard let last = result.last else { result.append(point); continue }
            if hypot(point.x - last.x, point.y - last.y) > 0.025 { result.append(point) }
        }
        return Array(result.prefix(24))
    }

    private struct Candidate {
        let points: [NormalizedPoint]
        let score: Double
    }
}

private struct EyeShapeDocument: FileDocument {
    static var readableContentTypes: [UTType] { [.json] }
    let design: EyeShapeDesign

    init(design: EyeShapeDesign) { self.design = design }
    init(configuration: ReadConfiguration) throws {
        design = try JSONDecoder().decode(EyeShapeDesign.self, from: configuration.file.regularFileContents ?? Data())
    }
    func fileWrapper(configuration: WriteConfiguration) throws -> FileWrapper {
        FileWrapper(regularFileWithContents: try design.encoded())
    }
}

private extension Color {
    init(hex: String) {
        let cleaned = hex.trimmingCharacters(in: CharacterSet.alphanumerics.inverted)
        let value = UInt64(cleaned, radix: 16) ?? 0x00f2ff
        self.init(
            red: Double((value >> 16) & 0xff) / 255,
            green: Double((value >> 8) & 0xff) / 255,
            blue: Double(value & 0xff) / 255
        )
    }

    var hexString: String {
        let color = UIColor(self)
        var red: CGFloat = 0
        var green: CGFloat = 0
        var blue: CGFloat = 0
        var alpha: CGFloat = 0
        guard color.getRed(&red, green: &green, blue: &blue, alpha: &alpha) else { return "#00f2ff" }
        return String(format: "#%02x%02x%02x", Int(red * 255), Int(green * 255), Int(blue * 255))
    }
}
