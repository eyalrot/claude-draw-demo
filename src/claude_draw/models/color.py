"""Color model for representing colors in various formats."""

from typing import Optional, Union, Self
from pydantic import field_validator, model_validator

from claude_draw.models.base import DrawModel
from claude_draw.models.validators import (
    validate_color_channel,
    validate_alpha_channel,
    validate_hex_color,
    clamp,
)


class Color(DrawModel):
    """A color that can be represented in RGB, RGBA, HSL, or hex format.
    
    The color is internally stored as RGBA values. Other formats are
    computed on demand.
    
    Attributes:
        r: Red channel (0-255)
        g: Green channel (0-255)
        b: Blue channel (0-255)
        a: Alpha channel (0.0-1.0)
    """
    
    r: int
    g: int
    b: int
    a: float = 1.0
    
    @field_validator('r', 'g', 'b')
    @classmethod
    def validate_rgb_channel(cls, value: int, info) -> int:
        """Validate RGB channel values."""
        return validate_color_channel(value, info.field_name)
    
    @field_validator('a')
    @classmethod
    def validate_alpha(cls, value: float) -> float:
        """Validate alpha channel value."""
        return validate_alpha_channel(value)
    
    @classmethod
    def from_rgb(cls, r: int, g: int, b: int, a: float = 1.0) -> "Color":
        """Create color from RGB(A) values.
        
        Args:
            r: Red channel (0-255)
            g: Green channel (0-255)
            b: Blue channel (0-255)
            a: Alpha channel (0.0-1.0)
            
        Returns:
            Color instance
        """
        return cls(r=r, g=g, b=b, a=a)
    
    @classmethod
    def from_hex(cls, hex_string: str) -> "Color":
        """Create color from hex string.
        
        Args:
            hex_string: Hex color string (e.g., "#FF0000", "FF0000", "#F00")
            
        Returns:
            Color instance
        """
        # Validate and normalize hex string
        hex_normalized = validate_hex_color(hex_string)
        
        # Parse RGB values
        r = int(hex_normalized[0:2], 16)
        g = int(hex_normalized[2:4], 16)
        b = int(hex_normalized[4:6], 16)
        
        return cls(r=r, g=g, b=b)
    
    @classmethod
    def from_hsl(cls, h: float, s: float, l: float, a: float = 1.0) -> "Color":
        """Create color from HSL values.
        
        Args:
            h: Hue (0-360)
            s: Saturation (0-100)
            l: Lightness (0-100)
            a: Alpha channel (0.0-1.0)
            
        Returns:
            Color instance
        """
        # Normalize values
        h = h % 360
        s = clamp(s, 0, 100) / 100
        l = clamp(l, 0, 100) / 100
        
        # Convert HSL to RGB
        if s == 0:
            # Achromatic (gray)
            r = g = b = int(l * 255)
        else:
            def hue_to_rgb(p: float, q: float, t: float) -> float:
                if t < 0:
                    t += 1
                if t > 1:
                    t -= 1
                if t < 1/6:
                    return p + (q - p) * 6 * t
                if t < 1/2:
                    return q
                if t < 2/3:
                    return p + (q - p) * (2/3 - t) * 6
                return p
            
            q = l * (1 + s) if l < 0.5 else l + s - l * s
            p = 2 * l - q
            
            r = int(hue_to_rgb(p, q, h/360 + 1/3) * 255)
            g = int(hue_to_rgb(p, q, h/360) * 255)
            b = int(hue_to_rgb(p, q, h/360 - 1/3) * 255)
        
        return cls(r=r, g=g, b=b, a=a)
    
    @classmethod
    def from_name(cls, name: str) -> "Color":
        """Create color from CSS color name.
        
        Args:
            name: Color name (e.g., "red", "blue", "green")
            
        Returns:
            Color instance
            
        Raises:
            ValueError: If color name is not recognized
        """
        # Common CSS color names
        color_map = {
            "black": "#000000",
            "white": "#FFFFFF",
            "red": "#FF0000",
            "green": "#008000",
            "blue": "#0000FF",
            "yellow": "#FFFF00",
            "cyan": "#00FFFF",
            "magenta": "#FF00FF",
            "gray": "#808080",
            "grey": "#808080",
            "darkgray": "#A9A9A9",
            "darkgrey": "#A9A9A9",
            "lightgray": "#D3D3D3",
            "lightgrey": "#D3D3D3",
            "orange": "#FFA500",
            "purple": "#800080",
            "brown": "#A52A2A",
            "pink": "#FFC0CB",
            "lime": "#00FF00",
            "navy": "#000080",
            "teal": "#008080",
            "maroon": "#800000",
            "olive": "#808000",
            "aqua": "#00FFFF",
            "fuchsia": "#FF00FF",
            "silver": "#C0C0C0",
        }
        
        name_lower = name.lower()
        
        # Special case for transparent
        if name_lower == "transparent":
            return cls(r=0, g=0, b=0, a=0.0)
        
        if name_lower not in color_map:
            raise ValueError(f"Unknown color name: {name}")
        
        return cls.from_hex(color_map[name_lower])
    
    def to_hex(self, include_alpha: bool = False) -> str:
        """Convert to hex string.
        
        Args:
            include_alpha: Whether to include alpha channel
            
        Returns:
            Hex string (e.g., "#FF0000" or "#FF0000FF")
        """
        if include_alpha:
            alpha_hex = format(int(self.a * 255), '02X')
            return f"#{self.r:02X}{self.g:02X}{self.b:02X}{alpha_hex}"
        return f"#{self.r:02X}{self.g:02X}{self.b:02X}"
    
    def to_rgb(self) -> tuple[int, int, int]:
        """Get RGB values as tuple.
        
        Returns:
            Tuple of (r, g, b)
        """
        return (self.r, self.g, self.b)
    
    def to_rgba(self) -> tuple[int, int, int, float]:
        """Get RGBA values as tuple.
        
        Returns:
            Tuple of (r, g, b, a)
        """
        return (self.r, self.g, self.b, self.a)
    
    def to_hsl(self) -> tuple[float, float, float]:
        """Convert to HSL values.
        
        Returns:
            Tuple of (h, s, l) where h is 0-360, s and l are 0-100
        """
        r = self.r / 255
        g = self.g / 255
        b = self.b / 255
        
        max_val = max(r, g, b)
        min_val = min(r, g, b)
        diff = max_val - min_val
        
        # Lightness
        l = (max_val + min_val) / 2
        
        if diff == 0:
            # Achromatic
            h = s = 0.0
        else:
            # Saturation
            s = diff / (2 - max_val - min_val) if l > 0.5 else diff / (max_val + min_val)
            
            # Hue
            if max_val == r:
                h = ((g - b) / diff + (6 if g < b else 0)) / 6
            elif max_val == g:
                h = ((b - r) / diff + 2) / 6
            else:
                h = ((r - g) / diff + 4) / 6
        
        return (h * 360, s * 100, l * 100)
    
    def to_hsla(self) -> tuple[float, float, float, float]:
        """Convert to HSLA values.
        
        Returns:
            Tuple of (h, s, l, a) where h is 0-360, s and l are 0-100, a is 0.0-1.0
        """
        h, s, l = self.to_hsl()
        return (h, s, l, self.a)
    
    def to_css(self) -> str:
        """Convert to CSS color string.
        
        Returns:
            CSS color string (e.g., "rgb(255, 0, 0)" or "rgba(255, 0, 0, 0.5)")
        """
        if self.a < 1.0:
            return f"rgba({self.r}, {self.g}, {self.b}, {self.a})"
        return f"rgb({self.r}, {self.g}, {self.b})"
    
    def __str__(self) -> str:
        """String representation of the color."""
        if self.a < 1.0:
            return f"Color({self.r}, {self.g}, {self.b}, {self.a})"
        return f"Color({self.r}, {self.g}, {self.b})"
    
    def __repr__(self) -> str:
        """Detailed string representation."""
        return f"Color(r={self.r}, g={self.g}, b={self.b}, a={self.a})"
    
    def __eq__(self, other: object) -> bool:
        """Check equality with another color."""
        if not isinstance(other, Color):
            return False
        return (self.r == other.r and 
                self.g == other.g and 
                self.b == other.b and 
                abs(self.a - other.a) < 1e-10)
    
    def __hash__(self) -> int:
        """Get hash of the color."""
        return hash((self.r, self.g, self.b, round(self.a, 10)))
    
    def lighten(self, amount: float) -> "Color":
        """Lighten the color by a percentage.
        
        Args:
            amount: Amount to lighten (0-100)
            
        Returns:
            New lightened color
        """
        h, s, l = self.to_hsl()
        l = min(100, l + amount)
        return Color.from_hsl(h, s, l, self.a)
    
    def darken(self, amount: float) -> "Color":
        """Darken the color by a percentage.
        
        Args:
            amount: Amount to darken (0-100)
            
        Returns:
            New darkened color
        """
        h, s, l = self.to_hsl()
        l = max(0, l - amount)
        return Color.from_hsl(h, s, l, self.a)
    
    def saturate(self, amount: float) -> "Color":
        """Increase saturation by a percentage.
        
        Args:
            amount: Amount to increase saturation (0-100)
            
        Returns:
            New saturated color
        """
        h, s, l = self.to_hsl()
        s = min(100, s + amount)
        return Color.from_hsl(h, s, l, self.a)
    
    def desaturate(self, amount: float) -> "Color":
        """Decrease saturation by a percentage.
        
        Args:
            amount: Amount to decrease saturation (0-100)
            
        Returns:
            New desaturated color
        """
        h, s, l = self.to_hsl()
        s = max(0, s - amount)
        return Color.from_hsl(h, s, l, self.a)
    
    def grayscale(self) -> "Color":
        """Convert to grayscale.
        
        Returns:
            Grayscale version of the color
        """
        # Use luminance formula for better perceptual grayscale
        gray = int(0.299 * self.r + 0.587 * self.g + 0.114 * self.b)
        return Color(r=gray, g=gray, b=gray, a=self.a)
    
    def invert(self) -> "Color":
        """Invert the color.
        
        Returns:
            Inverted color
        """
        return Color(
            r=255 - self.r,
            g=255 - self.g,
            b=255 - self.b,
            a=self.a
        )
    
    def mix(self, other: "Color", amount: float = 0.5) -> "Color":
        """Mix with another color.
        
        Args:
            other: Color to mix with
            amount: Mix ratio (0=this color, 1=other color)
            
        Returns:
            Mixed color
        """
        amount = clamp(amount, 0.0, 1.0)
        return Color(
            r=int(self.r + (other.r - self.r) * amount),
            g=int(self.g + (other.g - self.g) * amount),
            b=int(self.b + (other.b - self.b) * amount),
            a=self.a + (other.a - self.a) * amount
        )
    
    def with_alpha(self, alpha: float) -> "Color":
        """Create a copy with a different alpha value.
        
        Args:
            alpha: New alpha value (0.0-1.0)
            
        Returns:
            Color with new alpha
        """
        return Color(r=self.r, g=self.g, b=self.b, a=alpha)
    
    def rotate_hue(self, degrees: float) -> "Color":
        """Rotate hue by given degrees.
        
        Args:
            degrees: Degrees to rotate hue
            
        Returns:
            Color with rotated hue
        """
        h, s, l = self.to_hsl()
        h = (h + degrees) % 360
        return Color.from_hsl(h, s, l, self.a)
    
    def complement(self) -> "Color":
        """Get complementary color (opposite on color wheel).
        
        Returns:
            Complementary color
        """
        return self.rotate_hue(180)
    
    def is_light(self) -> bool:
        """Check if the color is light.
        
        Uses relative luminance calculation.
        
        Returns:
            True if the color is light
        """
        # Calculate relative luminance
        r = self.r / 255
        g = self.g / 255
        b = self.b / 255
        
        # Apply gamma correction
        r = r / 12.92 if r <= 0.03928 else ((r + 0.055) / 1.055) ** 2.4
        g = g / 12.92 if g <= 0.03928 else ((g + 0.055) / 1.055) ** 2.4
        b = b / 12.92 if b <= 0.03928 else ((b + 0.055) / 1.055) ** 2.4
        
        # Calculate luminance
        luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b
        return luminance > 0.5
    
    def is_dark(self) -> bool:
        """Check if the color is dark.
        
        Returns:
            True if the color is dark
        """
        return not self.is_light()