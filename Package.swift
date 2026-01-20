// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "termi",
    platforms: [
        .iOS(.v16)
    ],
    products: [
        .library(
            name: "termi",
            targets: ["termi"]
        )
    ],
    dependencies: [
        .package(url: "https://github.com/migueldeicaza/SwiftTerm.git", from: "1.9.0")
    ],
    targets: [
        .target(
            name: "termi",
            dependencies: ["SwiftTerm"],
            path: "App"
        )
    ]
)
