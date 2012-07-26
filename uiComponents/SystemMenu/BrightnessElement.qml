import Qt 4.7


MenuListEntry {
    id: brightnessElement

    property alias brightnessValue: brightnessSlider.setValue
    property bool active: true

    signal brightnessChanged(real value, bool save)

    selectable: false

    property int margin: 0
    property int spacing: 5

    content:
        Item {
            id: brightnessContent
            x: 4
            width: brightnessElement.width - 8
            height: brightnessElement.height



            Image {
                id: imgLess
                source: "/usr/palm/sysmgr/images/statusBar/brightness-less.png"
                x: margin
                y: brightnessElement.height/2 - height/2
            }

            Image {
                id: imgMore
                source: "/usr/palm/sysmgr/images/statusBar/brightness-more.png"
                x: brightnessContent.width - width - margin
                y: brightnessElement.height/2 - height/2
            }

            Slider {
                id: brightnessSlider
                width: brightnessContent.width - (imgLess.width + imgMore.width + 2 * margin + 2 * spacing)
                x: brightnessContent.width/2 - width/2
                y: brightnessContent.height/2 - height/2
                active: brightnessElement.active

                onValueChanged: {
                    brightnessChanged(value, done);
                }

                onSetFlickOverride: {
                    flickOverride(override)
                }
            }
        }
}
