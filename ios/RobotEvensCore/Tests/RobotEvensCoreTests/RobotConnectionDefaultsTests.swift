import XCTest
@testable import RobotEvensCore

final class RobotConnectionDefaultsTests: XCTestCase {
    func testDefaultBaseURLUsesRecordedRobotAddress() {
        XCTAssertEqual(RobotConnectionDefaults.defaultBaseURLString, "http://192.168.0.101")
    }

    func testNormalizedBaseURLTrimsWhitespaceAndAddsHTTPWhenNeeded() {
        XCTAssertEqual(
            RobotConnectionDefaults.normalizedBaseURL(" 192.168.0.103 "),
            "http://192.168.0.103"
        )
    }

    func testNormalizedBaseURLRemovesTrailingSlash() {
        XCTAssertEqual(
            RobotConnectionDefaults.normalizedBaseURL("http://192.168.0.103/"),
            "http://192.168.0.103"
        )
    }

    func testNormalizedBaseURLKeepsExistingScheme() {
        XCTAssertEqual(
            RobotConnectionDefaults.normalizedBaseURL("http://deskbot.local"),
            "http://deskbot.local"
        )
    }

    func testResolvedStoredURLMigratesOldDefaultAddress() {
        XCTAssertEqual(
            RobotConnectionDefaults.resolvedBaseURLString(stored: "http://192.168.0.103"),
            "http://192.168.0.101"
        )
    }

    func testResolvedStoredURLKeepsCustomAddress() {
        XCTAssertEqual(
            RobotConnectionDefaults.resolvedBaseURLString(stored: "http://192.168.0.88"),
            "http://192.168.0.88"
        )
    }
}
