param(
    [string]$OutputPath = (Join-Path $PSScriptRoot "..\installer\sce_sys\icon0.png")
)

Add-Type -AssemblyName System.Drawing

$size = 128
$source = [System.Drawing.Bitmap]::new($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($source)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$background = [System.Drawing.Color]::FromArgb(255, 43, 24, 17)
$cream = [System.Drawing.Color]::FromArgb(255, 255, 239, 204)
$graphics.Clear($background)

$creamBrush = [System.Drawing.SolidBrush]::new($cream)
$backgroundBrush = [System.Drawing.SolidBrush]::new($background)
$creamPen = [System.Drawing.Pen]::new($cream, 9)
$creamPen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$creamPen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
$creamPen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round

# Bold cup silhouette with open space around it; no outer ring or border.
$cup = [System.Drawing.Drawing2D.GraphicsPath]::new()
$cup.AddLine(24, 58, 85, 58)
$cup.AddLine(85, 58, 85, 82)
$cup.AddBezier(85, 82, 85, 99, 74, 108, 55, 108)
$cup.AddBezier(55, 108, 35, 108, 24, 98, 24, 80)
$cup.CloseFigure()
$graphics.FillPath($creamBrush, $cup)
$graphics.FillEllipse($backgroundBrush, 31, 52, 47, 14)
$graphics.DrawArc($creamPen, 75, 63, 39, 34, 265, 190)
$graphics.FillEllipse($creamBrush, 24, 104, 72, 8)

$graphics.DrawBezier($creamPen, 42, 43, 34, 34, 51, 30, 44, 20)
$graphics.DrawBezier($creamPen, 65, 43, 57, 34, 74, 30, 67, 20)

$indexed = [System.Drawing.Bitmap]::new($size, $size, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
$palette = $indexed.Palette
for ($i = 0; $i -lt 256; $i++) {
    $amount = $i / 255.0
    $red = [int]($background.R + (($cream.R - $background.R) * $amount))
    $green = [int]($background.G + (($cream.G - $background.G) * $amount))
    $blue = [int]($background.B + (($cream.B - $background.B) * $amount))
    $palette.Entries[$i] = [System.Drawing.Color]::FromArgb(255, $red, $green, $blue)
}
$indexed.Palette = $palette
$rectangle = [System.Drawing.Rectangle]::new(0, 0, $size, $size)
$data = $indexed.LockBits($rectangle, [System.Drawing.Imaging.ImageLockMode]::WriteOnly,
    [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
$pixels = [byte[]]::new($data.Stride * $size)
for ($y = 0; $y -lt $size; $y++) {
    for ($x = 0; $x -lt $size; $x++) {
        $color = $source.GetPixel($x, $y)
        $redRange = $cream.R - $background.R
        $index = [Math]::Round(255.0 * ($color.R - $background.R) / $redRange)
        $index = [Math]::Max(0, [Math]::Min(255, $index))
        $pixels[$y * $data.Stride + $x] = $index
    }
}
[System.Runtime.InteropServices.Marshal]::Copy($pixels, 0, $data.Scan0, $pixels.Length)
$indexed.UnlockBits($data)

$directory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Path $directory -Force | Out-Null
$indexed.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)

$indexed.Dispose()
$cup.Dispose()
$creamPen.Dispose()
$backgroundBrush.Dispose()
$creamBrush.Dispose()
$graphics.Dispose()
$source.Dispose()

