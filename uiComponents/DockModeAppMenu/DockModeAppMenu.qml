import Qt 4.7
import "../MenuContainer"

MenuContainer {
    id: dockModeAppMenu

    mainMenuItem.children: [DockModeAppMenuContainer]

    onMenuScrollStarted: {
        DockModeAppMenuContainer.mouseWasGrabbedByParent();
    }

    Connections {
        target: DockModeAppMenuContainer
        onSignalContainerSizeChanged: {
            mainMenuItem.width = DockModeAppMenuContainer.getWidth();
            mainMenuItem.height = DockModeAppMenuContainer.getHeight();
            mainMenuItem.y = 0;
        }
    }
    onVisibleChanged: {
        if(!visible) {
            // Quick trick to reset the scroll value of Flickable
            mainMenuItem.width  = 0;
            mainMenuItem.height = 0;

            mainMenuItem.width  = DockModeAppMenuContainer.getWidth();
            mainMenuItem.height = DockModeAppMenuContainer.getHeight();
        }
    }
}
