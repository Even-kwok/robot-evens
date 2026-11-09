import SwiftUI

struct CreatorStudioView: View {
    enum Studio: String, CaseIterable, Identifiable {
        case sound
        case expression

        var id: String { rawValue }
        var title: String { self == .sound ? "情绪声音" : "表情眼形" }
        var symbol: String { self == .sound ? "waveform.badge.plus" : "eyes" }
    }

    @Binding var robotBaseURLString: String
    @State private var studio: Studio = .sound

    var body: some View {
        VStack(spacing: 0) {
            Picker("创作功能", selection: $studio) {
                ForEach(Studio.allCases) { item in
                    Label(item.title, systemImage: item.symbol).tag(item)
                }
            }
            .pickerStyle(.segmented)
            .padding()

            switch studio {
            case .sound:
                EmotionSoundStudioView(robotBaseURLString: $robotBaseURLString)
            case .expression:
                ExpressionStudioView(robotBaseURLString: $robotBaseURLString)
            }
        }
        .navigationTitle("创作工作室")
        .navigationBarTitleDisplayMode(.inline)
    }
}
