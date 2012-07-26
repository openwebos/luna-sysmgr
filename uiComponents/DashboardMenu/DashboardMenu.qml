import Qt 4.7
import "../MenuContainer"

MenuContainer {
    id: dashboardMenu

    mainMenuItem.children: [DashboardContainer]

    onMenuScrollStarted: {
        DashboardContainer.mouseWasGrabbedByParent();
    }

    Connections {
        target: DashboardContainer
        onSignalContainerSizeChanged: {
            mainMenuItem.width = DashboardContainer.getWidth();
            mainMenuItem.height = DashboardContainer.getHeight();
            mainMenuItem.y = 0;
        }

        onSignalItemDragState: {
             scrollable = !itemBeingDragged;
        }
    }

    onVisibleChanged: {
        if(!visible) {
            // Quick trick to reset the scroll value of Flickable
            mainMenuItem.width  = 0;
            mainMenuItem.height = 0;

            mainMenuItem.width  = DashboardContainer.getWidth();
            mainMenuItem.height = DashboardContainer.getHeight();
        }
    }


}
