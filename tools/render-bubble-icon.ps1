param(
    [string]$OutputPath = (Join-Path $PSScriptRoot "..\installer\sce_sys\icon0.png")
)

Add-Type -AssemblyName System.Drawing

$size = 128
$source = [System.Drawing.Bitmap]::new($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$graphics = [System.Drawing.Graphics]::FromImage($source)
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$brown = [System.Drawing.Color]::FromArgb(255, 70, 35, 18)
$blue = [System.Drawing.Color]::FromArgb(255, 25, 145, 220)
$white = [System.Drawing.Color]::White
$graphics.Clear($brown)

$bluePen = [System.Drawing.Pen]::new($blue, 6)
$graphics.DrawEllipse($bluePen, 7, 7, 113, 113)
$whitePen = [System.Drawing.Pen]::new($white, 8)
$whitePen.StartCap = [System.Drawing.Drawing2D.LineCap]::Round
$whitePen.EndCap = [System.Drawing.Drawing2D.LineCap]::Round
$whitePen.LineJoin = [System.Drawing.Drawing2D.LineJoin]::Round

$cup = [System.Drawing.Drawing2D.GraphicsPath]::new()
$cup.AddLine(31, 55, 84, 55)
$cup.AddLine(84, 55, 84, 81)
$cup.AddBezier(84, 81, 84, 96, 74, 104, 58, 104)
$cup.AddBezier(58, 104, 41, 104, 31, 96, 31, 81)
$cup.CloseFigure()
$graphics.DrawPath($whitePen, $cup)
$graphics.DrawBezier($whitePen, 84, 63, 103, 61, 108, 70, 108, 79)
$graphics.DrawBezier($whitePen, 108, 79, 108, 89, 99, 94, 82, 93)
$graphics.DrawBezier($whitePen, 46, 44, 37, 35, 55, 32, 47, 23)
$graphics.DrawBezier($whitePen, 65, 44, 56, 35, 74, 32, 66, 23)

$indexed = [System.Drawing.Bitmap]::new($size, $size, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
$palette = $indexed.Palette
$palette.Entries[0] = $brown
$palette.Entries[1] = $blue
$palette.Entries[2] = $white
$indexed.Palette = $palette
$rectangle = [System.Drawing.Rectangle]::new(0, 0, $size, $size)
$data = $indexed.LockBits($rectangle, [System.Drawing.Imaging.ImageLockMode]::WriteOnly,
    [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
$pixels = [byte[]]::new($data.Stride * $size)
for ($y = 0; $y -lt $size; $y++) {
    for ($x = 0; $x -lt $size; $x++) {
        $color = $source.GetPixel($x, $y)
        if ($color.R -gt 230 -and $color.G -gt 230) { $index = 2 }
        elseif ($color.B -gt 150) { $index = 1 }
        else { $index = 0 }
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
$whitePen.Dispose()
$bluePen.Dispose()
$graphics.Dispose()
$source.Dispose()

