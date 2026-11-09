// swift-tools-version: 6.0

import PackageDescription

let package = Package(
    name: "RobotEvensCore",
    platforms: [
        .iOS(.v17),
        .macOS(.v14)
    ],
    products: [
        .library(
            name: "RobotEvensCore",
            targets: ["RobotEvensCore"]
        )
    ],
    targets: [
        .target(
            name: "RobotEvensCore"
        ),
        .testTarget(
            name: "RobotEvensCoreTests",
            dependencies: ["RobotEvensCore"]
        )
    ]
)
