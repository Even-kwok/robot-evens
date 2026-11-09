import Foundation
import SwiftUI

@MainActor
final class DesktopRobotConnectionViewModel: ObservableObject {
    enum ConnectionState: Equatable {
        case ready
        case connecting
        case connected
        case needsSetup
        case failed(String)
    }

    @Published private(set) var connectionState: ConnectionState = .ready
    @Published var firmwareAddress = "http://desktop-robot.local"
    @Published var setupAddress = "http://192.168.4.1"
    @Published var wifiName = ""
    @Published var wifiPassword = ""
    @Published private(set) var connectedWifiName = ""
    @Published private(set) var connectedFirmwareAddress = ""
    @Published private(set) var connectedFirmwareVersion = ""
    @Published private(set) var actionMessage = ""
    @Published private(set) var wifiSetupSucceeded = false

    private let storedFirmwareAddressKey = "desktopRobot.firmwareAddress"
    private let session: URLSession
    private var baseURL: URL?

    init() {
        let configuration = URLSessionConfiguration.ephemeral
        configuration.timeoutIntervalForRequest = 5
        configuration.timeoutIntervalForResource = 8
        session = URLSession(configuration: configuration)
        firmwareAddress = UserDefaults.standard.string(forKey: storedFirmwareAddressKey) ?? firmwareAddress
    }

    var isConnected: Bool {
        connectionState == .connected
    }

    var visibleFirmwareAddress: String {
        connectedFirmwareAddress.isEmpty ? normalizedAddressString(firmwareAddress) : connectedFirmwareAddress
    }

    var statusTitle: String {
        switch connectionState {
        case .ready:
            "Desktop Robot is ready"
        case .connecting:
            "Connecting to Desktop Robot"
        case .connected:
            "Desktop Robot is connected"
        case .needsSetup:
            "Desktop Robot needs setup"
        case .failed:
            "Desktop Robot is unavailable"
        }
    }

    var statusDetail: String {
        switch connectionState {
        case .ready, .connecting:
            "Make sure your robot is powered on and nearby."
        case .connected:
            connectedWifiName.isEmpty ? "Connected to the robot." : "Wi-Fi: \(connectedWifiName)"
        case .needsSetup:
            "Use the robot button to start Wi-Fi setup."
        case .failed(let message):
            message
        }
    }

    func connect() async {
        guard let candidate = normalizedURL(firmwareAddress) else {
            connectionState = .failed("Enter a valid firmware address.")
            return
        }

        connectionState = .connecting
        actionMessage = ""
        wifiSetupSucceeded = false
        do {
            let ping = try await request(baseURL: candidate, path: "/api/ping")
            guard ping["ok"] as? Bool != false else {
                connectionState = .failed("The address did not respond as a robot.")
                return
            }
            let wifi = try await request(baseURL: candidate, path: "/api/setup/status")
            baseURL = candidate
            connectedWifiName = wifi["ssid"] as? String ?? ""
            connectedFirmwareAddress = wifi["firmwareUrl"] as? String ?? normalizedAddressString(firmwareAddress)
            if let runtime = try? await request(baseURL: candidate, path: "/api/status"),
               let firmware = runtime["firmware"] as? [String: Any] {
                connectedFirmwareVersion = firmware["version"] as? String ?? ""
            }
            firmwareAddress = connectedFirmwareAddress
            UserDefaults.standard.set(firmwareAddress, forKey: storedFirmwareAddressKey)
            connectionState = .connected
        } catch {
            connectionState = .needsSetup
        }
    }

    func saveWifi() async -> Bool {
        guard let candidate = normalizedURL(setupAddress) else {
            actionMessage = "Enter a valid robot setup address."
            return false
        }
        let ssid = wifiName.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !ssid.isEmpty else {
            actionMessage = "Enter the Wi-Fi name."
            return false
        }

        actionMessage = "Saving Wi-Fi…"
        wifiSetupSucceeded = false
        do {
            let response = try await request(
                baseURL: candidate,
                path: "/api/setup/connect",
                method: "POST",
                body: ["ssid": ssid, "password": wifiPassword]
            )
            guard response["ok"] as? Bool != false,
                  response["connected"] as? Bool != false else {
                let reason = response["error"] as? String ?? response["statusName"] as? String ?? "Unknown"
                actionMessage = "Connection failed: \(reason)."
                return false
            }
            let nextAddress = response["firmwareUrl"] as? String
            connectedWifiName = response["ssid"] as? String ?? ssid
            if let nextAddress, !nextAddress.isEmpty {
                firmwareAddress = nextAddress
                connectedFirmwareAddress = nextAddress
                UserDefaults.standard.set(nextAddress, forKey: storedFirmwareAddressKey)
            }
            wifiPassword = ""
            actionMessage = "Connected successfully."
            wifiSetupSucceeded = true
            connectionState = .ready
            return true
        } catch {
            actionMessage = "Connection failed: No response."
            return false
        }
    }

    func disconnect() {
        baseURL = nil
        connectedWifiName = ""
        connectedFirmwareAddress = ""
        connectedFirmwareVersion = ""
        actionMessage = ""
        wifiSetupSucceeded = false
        connectionState = .ready
    }

    func setMood(_ mood: String) async {
        await perform(path: "/api/face/set", body: ["mood": mood], confirmation: "Expression updated.")
    }

    func playEmotion(_ name: String) async {
        await perform(path: "/api/emotion/play", body: ["name": name, "sound": true, "lights": true], confirmation: "Action started.")
    }

    func returnHome() async {
        await perform(path: "/api/motion/home", body: ["speed": 300], confirmation: "Robot returned to its standby pose.")
    }

    func setLight(color: String) async {
        await perform(path: "/api/rgb", body: ["color": color, "brightness": 180, "temporary": false], confirmation: "Light updated.")
    }

    private func perform(path: String, body: [String: Any], confirmation: String) async {
        guard let baseURL else {
            connectionState = .needsSetup
            return
        }
        actionMessage = "Sending…"
        do {
            let response = try await request(baseURL: baseURL, path: path, method: "POST", body: body)
            actionMessage = response["ok"] as? Bool == false ? (response["error"] as? String ?? "Robot did not accept the request.") : confirmation
        } catch {
            connectionState = .needsSetup
            actionMessage = "Connection to the robot was lost."
        }
    }

    private func request(
        baseURL: URL,
        path: String,
        method: String = "GET",
        body: [String: Any] = [:]
    ) async throws -> [String: Any] {
        var request = URLRequest(url: baseURL.appending(path: path))
        request.httpMethod = method
        request.setValue("application/json", forHTTPHeaderField: "Accept")
        if !body.isEmpty {
            request.httpBody = try JSONSerialization.data(withJSONObject: body, options: [])
            request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        }
        let (data, response) = try await session.data(for: request)
        guard let http = response as? HTTPURLResponse, (200...299).contains(http.statusCode) else {
            throw URLError(.badServerResponse)
        }
        guard let json = try JSONSerialization.jsonObject(with: data) as? [String: Any] else {
            throw URLError(.cannotParseResponse)
        }
        return json
    }

    private func normalizedURL(_ value: String) -> URL? {
        URL(string: normalizedAddressString(value)).flatMap { $0.host == nil ? nil : $0 }
    }

    private func normalizedAddressString(_ value: String) -> String {
        let trimmed = value.trimmingCharacters(in: .whitespacesAndNewlines)
        let withScheme = trimmed.hasPrefix("http://") || trimmed.hasPrefix("https://") ? trimmed : "http://\(trimmed)"
        return withScheme.hasSuffix("/") ? String(withScheme.dropLast()) : withScheme
    }
}

struct DesktopRobotAppView: View {
    @StateObject private var model = DesktopRobotConnectionViewModel()

    var body: some View {
        Group {
            if model.isConnected {
                DesktopRobotTabView(model: model)
            } else {
                DesktopRobotWelcomeView(model: model)
            }
        }
        .tint(.blue)
    }
}

private struct DesktopRobotWelcomeView: View {
    @ObservedObject var model: DesktopRobotConnectionViewModel
    @State private var isShowingSetup = false
    @State private var isEditingAddress = false

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            Spacer(minLength: 42)

            Text("Connect your robot")
                .font(.system(size: 34, weight: .bold, design: .default))
                .foregroundStyle(.primary)

            Text(model.statusDetail)
                .font(.body)
                .foregroundStyle(.secondary)
                .padding(.top, 8)

            Spacer(minLength: 54)

            HStack {
                Spacer()
                Image(systemName: "dot.radiowaves.left.and.right")
                    .font(.system(size: 60, weight: .light))
                    .foregroundStyle(.blue)
                    .accessibilityHidden(true)
                Spacer()
            }

            Spacer(minLength: 46)

            Text(model.statusTitle)
                .font(.title3.weight(.semibold))
                .frame(maxWidth: .infinity, alignment: .center)

            Text(model.visibleFirmwareAddress)
                .font(.footnote)
                .foregroundStyle(.secondary)
                .frame(maxWidth: .infinity, alignment: .center)
                .padding(.top, 5)

            if !model.actionMessage.isEmpty {
                Text(model.actionMessage)
                    .font(.footnote.weight(.medium))
                    .foregroundStyle(model.wifiSetupSucceeded ? .green : .secondary)
                    .frame(maxWidth: .infinity, alignment: .center)
                    .padding(.top, 8)
            }

            if isEditingAddress {
                TextField("Firmware address", text: $model.firmwareAddress)
                    .textInputAutocapitalization(.never)
                    .autocorrectionDisabled()
                    .keyboardType(.URL)
                    .textFieldStyle(.roundedBorder)
                    .padding(.top, 18)
            }

            Button {
                Task { await model.connect() }
            } label: {
                HStack {
                    if model.connectionState == .connecting {
                        ProgressView().tint(.white)
                    }
                    Text(model.connectionState == .connecting ? "Connecting" : "Connect")
                }
                .frame(maxWidth: .infinity)
            }
            .buttonStyle(.borderedProminent)
            .controlSize(.large)
            .disabled(model.connectionState == .connecting)
            .padding(.top, 26)

            Button {
                isShowingSetup = true
            } label: {
                VStack(spacing: 4) {
                    Text("Set up with robot button")
                        .font(.body.weight(.medium))
                    Text("Press the power button three times to start Wi-Fi setup.")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                        .multilineTextAlignment(.center)
                }
                .frame(maxWidth: .infinity)
                .padding(.vertical, 18)
            }
            .buttonStyle(.plain)

            Button(isEditingAddress ? "Hide firmware address" : "Use a different firmware address") {
                withAnimation { isEditingAddress.toggle() }
            }
            .font(.footnote)
            .frame(maxWidth: .infinity)
            .padding(.bottom, 12)
        }
        .padding(.horizontal, 24)
        .sheet(isPresented: $isShowingSetup) {
            DesktopRobotSetupView(model: model)
        }
    }
}

private struct DesktopRobotSetupView: View {
    @ObservedObject var model: DesktopRobotConnectionViewModel
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        NavigationStack {
            Form {
                Section("1. Start setup on the robot") {
                    Text("Press the robot’s power button three times. Its display will show the Wi-Fi status, firmware address, and setup hotspot details.")
                    Text("Join the setup hotspot shown on the robot, then return to this app.")
                        .foregroundStyle(.secondary)
                }

                Section("2. Connect Desktop Robot to Wi-Fi") {
                    TextField("Wi-Fi name", text: $model.wifiName)
                        .textInputAutocapitalization(.never)
                        .autocorrectionDisabled()
                    SecureField("Wi-Fi password", text: $model.wifiPassword)
                    Text("Desktop Robot uses the name you enter. It does not scan nearby Wi-Fi networks.")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                }

                Section("Robot setup page") {
                    TextField("Robot setup address", text: $model.setupAddress)
                        .textInputAutocapitalization(.never)
                        .autocorrectionDisabled()
                        .keyboardType(.URL)
                }

                if !model.actionMessage.isEmpty {
                    Section {
                        Label(model.actionMessage, systemImage: model.wifiSetupSucceeded ? "checkmark.circle.fill" : "exclamationmark.triangle.fill")
                            .foregroundStyle(model.wifiSetupSucceeded ? .green : .secondary)
                        if model.wifiSetupSucceeded {
                            LabeledContent("Wi-Fi", value: model.connectedWifiName)
                            LabeledContent("Firmware", value: model.visibleFirmwareAddress)
                            Text("Return your phone to its normal Wi-Fi, then tap Connect.")
                                .font(.footnote)
                                .foregroundStyle(.secondary)
                        }
                    }
                }
            }
            .navigationTitle("Set up Wi-Fi")
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Done") { dismiss() }
                }
                ToolbarItem(placement: .confirmationAction) {
                    Button("Save") {
                        Task {
                            _ = await model.saveWifi()
                        }
                    }
                    .disabled(model.wifiName.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty)
                }
            }
        }
    }
}

private struct DesktopRobotTabView: View {
    @ObservedObject var model: DesktopRobotConnectionViewModel

    var body: some View {
        TabView {
            DesktopRobotHomeView(model: model)
                .tabItem { Label("Robot", systemImage: "cpu") }
            DesktopRobotExpressionsView(model: model)
                .tabItem { Label("Expression", systemImage: "face.smiling") }
            DesktopRobotActionsView(model: model)
                .tabItem { Label("Actions", systemImage: "sparkles") }
            DesktopRobotSettingsView(model: model)
                .tabItem { Label("Settings", systemImage: "gearshape") }
        }
    }
}

private struct DesktopRobotHomeView: View {
    @ObservedObject var model: DesktopRobotConnectionViewModel

    var body: some View {
        NavigationStack {
            List {
                Section {
                    Label("Desktop Robot is connected", systemImage: "checkmark.circle.fill")
                        .foregroundStyle(.green)
                    LabeledContent("Wi-Fi", value: model.connectedWifiName.isEmpty ? "Connected" : model.connectedWifiName)
                    LabeledContent("Firmware", value: model.visibleFirmwareAddress)
                }

                Section("Quick controls") {
                    Button { Task { await model.returnHome() } } label: {
                        Label("Return to standby", systemImage: "house")
                    }
                    Button { Task { await model.setLight(color: "#ffffff") } } label: {
                        Label("Warm white light", systemImage: "lightbulb")
                    }
                }

                if !model.actionMessage.isEmpty {
                    Section { Text(model.actionMessage).foregroundStyle(.secondary) }
                }
            }
            .navigationTitle("Desktop Robot")
        }
    }
}

private struct DesktopRobotExpressionsView: View {
    @ObservedObject var model: DesktopRobotConnectionViewModel

    private let moods = [("Happy", "happy"), ("Love", "love"), ("Thinking", "thinking"), ("Curious", "curious"), ("Sleepy", "sleeping"), ("Neutral", "neutral")]

    var body: some View {
        NavigationStack {
            List {
                Section("Choose an expression") {
                    ForEach(moods, id: \.1) { mood in
                        Button(mood.0) { Task { await model.setMood(mood.1) } }
                    }
                }
                if !model.actionMessage.isEmpty {
                    Section { Text(model.actionMessage).foregroundStyle(.secondary) }
                }
            }
            .navigationTitle("Expression")
        }
    }
}

private struct DesktopRobotActionsView: View {
    @ObservedObject var model: DesktopRobotConnectionViewModel

    var body: some View {
        NavigationStack {
            List {
                Section("Robot actions") {
                    Button { Task { await model.playEmotion("happy") } } label: {
                        Label("Celebrate", systemImage: "hands.clap")
                    }
                    Button { Task { await model.playEmotion("surprised") } } label: {
                        Label("Surprise", systemImage: "exclamationmark.bubble")
                    }
                    Button { Task { await model.playEmotion("sleepWake") } } label: {
                        Label("Rest", systemImage: "moon")
                    }
                    Button(role: .destructive) { Task { await model.returnHome() } } label: {
                        Label("Stop and return to standby", systemImage: "stop.circle")
                    }
                }

                Section("Lights") {
                    Button("Blue") { Task { await model.setLight(color: "#2f6bff") } }
                    Button("Pink") { Task { await model.setLight(color: "#ff4fd8") } }
                    Button("Green") { Task { await model.setLight(color: "#00ff66") } }
                }

                if !model.actionMessage.isEmpty {
                    Section { Text(model.actionMessage).foregroundStyle(.secondary) }
                }
            }
            .navigationTitle("Actions")
        }
    }
}

private struct DesktopRobotSettingsView: View {
    @ObservedObject var model: DesktopRobotConnectionViewModel
    @State private var isShowingSetup = false

    var body: some View {
        NavigationStack {
            List {
                Section("Connection") {
                    LabeledContent("Firmware address", value: model.visibleFirmwareAddress)
                    Button("Set up Wi-Fi") { isShowingSetup = true }
                    Button("Disconnect", role: .destructive) { model.disconnect() }
                }
                Section("About") {
                    LabeledContent("Robot name", value: "Desktop Robot")
                    LabeledContent("Firmware version", value: model.connectedFirmwareVersion.isEmpty ? "V0.81 expected" : model.connectedFirmwareVersion)
                    Text("Use the firmware address shown on the robot if automatic connection is unavailable.")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                }
            }
            .navigationTitle("Settings")
            .sheet(isPresented: $isShowingSetup) {
                DesktopRobotSetupView(model: model)
            }
        }
    }
}
