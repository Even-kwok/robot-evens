import Foundation

public enum RobotConnectionDefaults {
    public static let defaultBaseURLString = "http://192.168.0.101"

    private static let staleBaseURLStrings = [
        "http://192.168.0.103"
    ]

    public static func normalizedBaseURL(_ value: String) -> String {
        let trimmed = value.trimmingCharacters(in: .whitespacesAndNewlines)
        let withScheme: String
        if trimmed.hasPrefix("http://") || trimmed.hasPrefix("https://") {
            withScheme = trimmed
        } else {
            withScheme = "http://\(trimmed)"
        }

        if withScheme.count > "http://".count, withScheme.hasSuffix("/") {
            return String(withScheme.dropLast())
        }
        return withScheme
    }

    public static func resolvedBaseURLString(stored: String?) -> String {
        guard let stored, !stored.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty else {
            return defaultBaseURLString
        }

        let normalizedStored = normalizedBaseURL(stored)
        if staleBaseURLStrings.contains(normalizedStored) {
            return defaultBaseURLString
        }
        return normalizedStored
    }
}
