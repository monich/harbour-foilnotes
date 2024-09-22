.pragma library
.import Sailfish.Silica 1.0 as Silica

var _fontPixesSizes = {
    "tiny":       Silica.Theme.fontSizeTiny,
    "extraSmall": Silica.Theme.fontSizeExtraSmall,
    "small":      Silica.Theme.fontSizeSmall,
    "medium":     Silica.Theme.fontSizeMedium,
    "large":      Silica.Theme.fontSizeLarge,
    "extraLarge": Silica.Theme.fontSizeExtraLarge,
    "huge":       Silica.Theme.fontSizeHuge
}

function fontPixelSize(config, defaultSize) {
    var size = _fontPixesSizes[config]
    return !!size ? size : defaultSize
}
