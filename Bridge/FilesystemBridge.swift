import Foundation

class FilesystemBridge {
    static let shared = FilesystemBridge()
    
    private init() {}
    
    func initialize() {
        let documentsPath = FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
        let rootfsPath = documentsPath.appendingPathComponent("rootfs")
        let dataPath = rootfsPath.appendingPathComponent("data")
        let dbPath = rootfsPath.appendingPathComponent("meta.db")
        
        try? FileManager.default.createDirectory(at: rootfsPath, withIntermediateDirectories: true)
        try? FileManager.default.createDirectory(at: dataPath, withIntermediateDirectories: true)
        
        print("FilesystemBridge: Initialized at \(rootfsPath.path)")
        print("FilesystemBridge: DB path: \(dbPath.path)")
        print("FilesystemBridge: Data path: \(dataPath.path)")
    }
}
