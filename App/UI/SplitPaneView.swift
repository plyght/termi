import SwiftUI

struct SplitPaneView: View {
    @EnvironmentObject var sessionManager: SessionManager
    @State private var splitOrientation: SplitOrientation = .horizontal
    @State private var splitRatio: CGFloat = 0.5
    
    enum SplitOrientation {
        case horizontal, vertical
    }
    
    var body: some View {
        GeometryReader { geometry in
            if sessionManager.sessions.count >= 2 {
                switch splitOrientation {
                case .horizontal:
                    HStack(spacing: 1) {
                        if let firstSession = sessionManager.sessions.first {
                            TerminalView(session: firstSession)
                                .frame(width: geometry.size.width * splitRatio)
                        }
                        
                        SplitDivider(orientation: .vertical, ratio: $splitRatio)
                        
                        if sessionManager.sessions.count > 1 {
                            TerminalView(session: sessionManager.sessions[1])
                        }
                    }
                    
                case .vertical:
                    VStack(spacing: 1) {
                        if let firstSession = sessionManager.sessions.first {
                            TerminalView(session: firstSession)
                                .frame(height: geometry.size.height * splitRatio)
                        }
                        
                        SplitDivider(orientation: .horizontal, ratio: $splitRatio)
                        
                        if sessionManager.sessions.count > 1 {
                            TerminalView(session: sessionManager.sessions[1])
                        }
                    }
                }
            } else if let session = sessionManager.sessions.first {
                TerminalView(session: session)
            }
        }
    }
}

struct SplitDivider: View {
    enum Orientation {
        case horizontal, vertical
    }
    
    let orientation: Orientation
    @Binding var ratio: CGFloat
    @EnvironmentObject var themeManager: ThemeManager
    @State private var isDragging = false
    
    var body: some View {
        Rectangle()
            .fill(themeManager.currentTheme.foregroundColor.opacity(0.2))
            .frame(
                width: orientation == .vertical ? 2 : nil,
                height: orientation == .horizontal ? 2 : nil
            )
            .gesture(
                DragGesture()
                    .onChanged { value in
                        isDragging = true
                        updateRatio(with: value.translation)
                    }
                    .onEnded { _ in
                        isDragging = false
                    }
            )
    }
    
    private func updateRatio(with translation: CGSize) {
        switch orientation {
        case .horizontal:
            ratio = max(0.2, min(0.8, ratio + translation.height / 500))
        case .vertical:
            ratio = max(0.2, min(0.8, ratio + translation.width / 500))
        }
    }
}
