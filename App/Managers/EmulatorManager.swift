import Foundation
import Combine

class EmulatorManager: ObservableObject {
    @Published var isRunning: Bool = false
    @Published var output: String = ""
    
    private var emulatorBridge: EmulatorBridge?
    
    init() {
        self.emulatorBridge = EmulatorBridge()
    }
    
    func start() async {
        guard !isRunning else { return }
        
        isRunning = true
        
        await Task {
            emulatorBridge?.start()
        }.value
    }
    
    func stop() {
        emulatorBridge?.stop()
        isRunning = false
    }
    
    func sendInput(_ input: String) {
        emulatorBridge?.sendInput(input)
    }
}
