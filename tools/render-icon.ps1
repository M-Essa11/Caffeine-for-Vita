param(
    [string]$OutputPath = (Join-Path $PSScriptRoot "..\assets\caffeine.png")
)

Add-Type -AssemblyName System.Drawing

$bitmap = [System.Drawing.Bitmap]::new(128, 128, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.Clear([System.Drawing.Color]::Transparent)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

$pen = [System.Drawing.Pen]::new([System.Drawing.Color]::White, 9)
$pen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$pen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
$pen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round

$cup = [System.Drawing.Drawing2D.GraphicsPath]::new()
$cup.StartFigure()
$cup.AddLine(29, 52, 90, 52)
$cup.AddLine(90, 52, 90, 82)
$cup.AddBezier(90, 82, 90, 99, 78, 110, 60, 110)
$cup.AddBezier(60, 110, 41, 110, 29, 99, 29, 82)
$cup.CloseFigure()
$graphics.DrawPath($pen, $cup)

$handle = [System.Drawing.Drawing2D.GraphicsPath]::new()
$handle.AddBezier(90, 61, 108, 59, 116, 67, 116, 78)
$handle.AddBezier(116, 78, 116, 89, 107, 96, 86, 95)
$graphics.DrawPath($pen, $handle)

$steamOne = [System.Drawing.Drawing2D.GraphicsPath]::new()
$steamOne.AddBezier(46, 39, 35, 29, 56, 27, 46, 17)
$graphics.DrawPath($pen, $steamOne)
$steamTwo = [System.Drawing.Drawing2D.GraphicsPath]::new()
$steamTwo.AddBezier(67, 39, 56, 29, 77, 27, 67, 17)
$graphics.DrawPath($pen, $steamTwo)

$outputDirectory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$indexed = [System.Drawing.Bitmap]::new(128, 128, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
$palette = $indexed.Palette
for ($i = 0; $i -lt 256; $i++) {
    $palette.Entries[$i] = [System.Drawing.Color]::FromArgb($i, 255, 255, 255)
}
$indexed.Palette = $palette

$rectangle = [System.Drawing.Rectangle]::new(0, 0, 128, 128)
$data = $indexed.LockBits(
    $rectangle,
    [System.Drawing.Imaging.ImageLockMode]::WriteOnly,
    [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
$pixels = [byte[]]::new($data.Stride * 128)
for ($y = 0; $y -lt 128; $y++) {
    for ($x = 0; $x -lt 128; $x++) {
        $pixels[$y * $data.Stride + $x] = $bitmap.GetPixel($x, $y).A
    }
}
[System.Runtime.InteropServices.Marshal]::Copy($pixels, 0, $data.Scan0, $pixels.Length)
$indexed.UnlockBits($data)
$indexed.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)

$steamTwo.Dispose()
$steamOne.Dispose()
$handle.Dispose()
$cup.Dispose()
$pen.Dispose()
$graphics.Dispose()
$bitmap.Dispose()
$indexed.Dispose()
