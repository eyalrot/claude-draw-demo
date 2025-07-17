"""Tests for Color model."""

import pytest
from pydantic import ValidationError

from claude_draw.models.color import Color


class TestColor:
    """Test Color model."""
    
    def test_creation_rgb(self):
        """Test creating a color with RGB values."""
        color = Color(r=255, g=0, b=0)
        assert color.r == 255
        assert color.g == 0
        assert color.b == 0
        assert color.a == 1.0
    
    def test_creation_rgba(self):
        """Test creating a color with RGBA values."""
        color = Color(r=255, g=0, b=0, a=0.5)
        assert color.r == 255
        assert color.g == 0
        assert color.b == 0
        assert color.a == 0.5
    
    def test_invalid_rgb_values(self):
        """Test that invalid RGB values are rejected."""
        # Negative values
        with pytest.raises(ValidationError):
            Color(r=-1, g=0, b=0)
        
        # Values > 255
        with pytest.raises(ValidationError):
            Color(r=256, g=0, b=0)
        
        # Non-integer values (strict mode)
        with pytest.raises(ValidationError):
            Color(r=255.5, g=0, b=0)  # type: ignore
    
    def test_invalid_alpha_values(self):
        """Test that invalid alpha values are rejected."""
        # Negative alpha
        with pytest.raises(ValidationError):
            Color(r=255, g=0, b=0, a=-0.1)
        
        # Alpha > 1
        with pytest.raises(ValidationError):
            Color(r=255, g=0, b=0, a=1.1)
    
    def test_from_rgb(self):
        """Test creating color from RGB values."""
        color = Color.from_rgb(255, 128, 0)
        assert color.r == 255
        assert color.g == 128
        assert color.b == 0
        assert color.a == 1.0
        
        # With alpha
        color2 = Color.from_rgb(255, 128, 0, 0.75)
        assert color2.a == 0.75
    
    def test_from_hex(self):
        """Test creating color from hex string."""
        # 6-digit hex
        color1 = Color.from_hex("#FF0000")
        assert color1.r == 255
        assert color1.g == 0
        assert color1.b == 0
        
        # Without #
        color2 = Color.from_hex("00FF00")
        assert color2.r == 0
        assert color2.g == 255
        assert color2.b == 0
        
        # 3-digit hex
        color3 = Color.from_hex("#F0A")
        assert color3.r == 255
        assert color3.g == 0
        assert color3.b == 170
        
        # Lowercase
        color4 = Color.from_hex("#aabbcc")
        assert color4.r == 170
        assert color4.g == 187
        assert color4.b == 204
    
    def test_from_hsl(self):
        """Test creating color from HSL values."""
        # Pure red
        color1 = Color.from_hsl(0, 100, 50)
        assert color1.r == 255
        assert color1.g == 0
        assert color1.b == 0
        
        # Pure green
        color2 = Color.from_hsl(120, 100, 50)
        assert color2.r == 0
        assert color2.g == 255
        assert color2.b == 0
        
        # Pure blue
        color3 = Color.from_hsl(240, 100, 50)
        assert color3.r == 0
        assert color3.g == 0
        assert color3.b == 255
        
        # Gray (no saturation)
        color4 = Color.from_hsl(0, 0, 50)
        assert color4.r == 127
        assert color4.g == 127
        assert color4.b == 127
        
        # With alpha
        color5 = Color.from_hsl(180, 100, 50, 0.5)
        assert color5.a == 0.5
    
    def test_from_name(self):
        """Test creating color from CSS color name."""
        # Basic colors
        assert Color.from_name("red") == Color(r=255, g=0, b=0)
        assert Color.from_name("green") == Color(r=0, g=128, b=0)
        assert Color.from_name("blue") == Color(r=0, g=0, b=255)
        
        # Case insensitive
        assert Color.from_name("RED") == Color(r=255, g=0, b=0)
        assert Color.from_name("Red") == Color(r=255, g=0, b=0)
        
        # Other colors
        assert Color.from_name("black") == Color(r=0, g=0, b=0)
        assert Color.from_name("white") == Color(r=255, g=255, b=255)
        assert Color.from_name("gray") == Color(r=128, g=128, b=128)
        
        # Transparent
        transparent = Color.from_name("transparent")
        assert transparent.r == 0
        assert transparent.g == 0
        assert transparent.b == 0
        assert transparent.a == 0.0
        
        # Unknown color
        with pytest.raises(ValueError, match="Unknown color name"):
            Color.from_name("notacolor")
    
    def test_to_hex(self):
        """Test converting to hex string."""
        color = Color(r=255, g=0, b=0)
        assert color.to_hex() == "#FF0000"
        
        # With alpha
        color2 = Color(r=255, g=0, b=0, a=0.5)
        assert color2.to_hex() == "#FF0000"
        assert color2.to_hex(include_alpha=True) == "#FF00007F"
        
        # Different color
        color3 = Color(r=170, g=187, b=204)
        assert color3.to_hex() == "#AABBCC"
    
    def test_to_rgb(self):
        """Test getting RGB tuple."""
        color = Color(r=255, g=128, b=0)
        assert color.to_rgb() == (255, 128, 0)
    
    def test_to_rgba(self):
        """Test getting RGBA tuple."""
        color = Color(r=255, g=128, b=0, a=0.75)
        assert color.to_rgba() == (255, 128, 0, 0.75)
    
    def test_to_hsl(self):
        """Test converting to HSL."""
        # Pure red
        color1 = Color(r=255, g=0, b=0)
        h, s, l = color1.to_hsl()
        assert abs(h - 0) < 0.1
        assert abs(s - 100) < 0.1
        assert abs(l - 50) < 0.1
        
        # Pure green
        color2 = Color(r=0, g=255, b=0)
        h, s, l = color2.to_hsl()
        assert abs(h - 120) < 0.1
        assert abs(s - 100) < 0.1
        assert abs(l - 50) < 0.1
        
        # Gray
        color3 = Color(r=128, g=128, b=128)
        h, s, l = color3.to_hsl()
        assert s == 0  # No saturation
        assert abs(l - 50.2) < 0.1
    
    def test_to_hsla(self):
        """Test converting to HSLA."""
        color = Color(r=255, g=0, b=0, a=0.5)
        h, s, l, a = color.to_hsla()
        assert abs(h - 0) < 0.1
        assert abs(s - 100) < 0.1
        assert abs(l - 50) < 0.1
        assert a == 0.5
    
    def test_to_css(self):
        """Test converting to CSS string."""
        # Opaque color
        color1 = Color(r=255, g=0, b=0)
        assert color1.to_css() == "rgb(255, 0, 0)"
        
        # With alpha
        color2 = Color(r=255, g=0, b=0, a=0.5)
        assert color2.to_css() == "rgba(255, 0, 0, 0.5)"
    
    def test_string_representation(self):
        """Test string representations."""
        color1 = Color(r=255, g=0, b=0)
        assert str(color1) == "Color(255, 0, 0)"
        assert repr(color1) == "Color(r=255, g=0, b=0, a=1.0)"
        
        color2 = Color(r=255, g=0, b=0, a=0.5)
        assert str(color2) == "Color(255, 0, 0, 0.5)"
        assert repr(color2) == "Color(r=255, g=0, b=0, a=0.5)"
    
    def test_equality(self):
        """Test color equality."""
        color1 = Color(r=255, g=0, b=0)
        color2 = Color(r=255, g=0, b=0)
        color3 = Color(r=0, g=255, b=0)
        
        assert color1 == color2
        assert color1 != color3
        assert color1 != "not a color"
        
        # Alpha comparison
        color4 = Color(r=255, g=0, b=0, a=0.5)
        color5 = Color(r=255, g=0, b=0, a=0.5)
        assert color4 == color5
        assert color1 != color4
    
    def test_hash(self):
        """Test color hashing."""
        color1 = Color(r=255, g=0, b=0)
        color2 = Color(r=255, g=0, b=0)
        color3 = Color(r=0, g=255, b=0)
        
        assert hash(color1) == hash(color2)
        assert hash(color1) != hash(color3)
        
        # Can be used in sets
        color_set = {color1, color2, color3}
        assert len(color_set) == 2
    
    def test_serialization(self):
        """Test JSON serialization."""
        color = Color(r=255, g=128, b=64, a=0.75)
        
        # To dict
        data = color.to_dict()
        assert data == {"r": 255, "g": 128, "b": 64, "a": 0.75}
        
        # From dict
        color2 = Color.from_dict(data)
        assert color2 == color
        
        # To JSON
        json_str = color.to_json()
        assert '"r":255' in json_str
        assert '"g":128' in json_str
        assert '"b":64' in json_str
        assert '"a":0.75' in json_str
        
        # From JSON
        color3 = Color.from_json(json_str)
        assert color3 == color
    
    def test_lighten(self):
        """Test color lightening."""
        color = Color(r=128, g=128, b=128)  # Gray
        lighter = color.lighten(20)
        
        # Should be lighter - compare lightness values
        _, _, l1 = color.to_hsl()
        _, _, l2 = lighter.to_hsl()
        assert l2 > l1
        
        # Alpha should be preserved
        color_with_alpha = Color(r=128, g=128, b=128, a=0.5)
        lighter_with_alpha = color_with_alpha.lighten(20)
        assert lighter_with_alpha.a == 0.5
    
    def test_darken(self):
        """Test color darkening."""
        color = Color(r=200, g=200, b=200)  # Light gray
        darker = color.darken(20)
        
        # Should be darker
        assert darker.is_dark() if color.is_light() else darker != color
        
        # Alpha should be preserved
        color_with_alpha = Color(r=200, g=200, b=200, a=0.5)
        darker_with_alpha = color_with_alpha.darken(20)
        assert darker_with_alpha.a == 0.5
    
    def test_saturate(self):
        """Test color saturation increase."""
        color = Color(r=150, g=100, b=100)  # Muted red
        saturated = color.saturate(30)
        
        # Should be more saturated
        _, s1, _ = color.to_hsl()
        _, s2, _ = saturated.to_hsl()
        assert s2 > s1
        
        # Alpha should be preserved
        color_with_alpha = Color(r=150, g=100, b=100, a=0.5)
        saturated_with_alpha = color_with_alpha.saturate(30)
        assert saturated_with_alpha.a == 0.5
    
    def test_desaturate(self):
        """Test color saturation decrease."""
        color = Color(r=255, g=0, b=0)  # Pure red
        desaturated = color.desaturate(50)
        
        # Should be less saturated
        _, s1, _ = color.to_hsl()
        _, s2, _ = desaturated.to_hsl()
        assert s2 < s1
        
        # Alpha should be preserved
        color_with_alpha = Color(r=255, g=0, b=0, a=0.5)
        desaturated_with_alpha = color_with_alpha.desaturate(50)
        assert desaturated_with_alpha.a == 0.5
    
    def test_grayscale(self):
        """Test grayscale conversion."""
        color = Color(r=255, g=0, b=0)  # Red
        gray = color.grayscale()
        
        # Should be gray (R=G=B)
        assert gray.r == gray.g == gray.b
        
        # Alpha should be preserved
        color_with_alpha = Color(r=255, g=0, b=0, a=0.5)
        gray_with_alpha = color_with_alpha.grayscale()
        assert gray_with_alpha.a == 0.5
        
        # Test luminance formula
        expected_gray = int(0.299 * 255 + 0.587 * 0 + 0.114 * 0)
        assert gray.r == expected_gray
    
    def test_invert(self):
        """Test color inversion."""
        color = Color(r=255, g=128, b=0)
        inverted = color.invert()
        
        assert inverted.r == 0
        assert inverted.g == 127
        assert inverted.b == 255
        assert inverted.a == color.a
        
        # Double inversion should return original
        double_inverted = inverted.invert()
        assert double_inverted.r == color.r
        assert double_inverted.g == color.g
        assert double_inverted.b == color.b
    
    def test_mix(self):
        """Test color mixing."""
        red = Color(r=255, g=0, b=0)
        blue = Color(r=0, g=0, b=255)
        
        # 50% mix should be purple
        purple = red.mix(blue, 0.5)
        assert purple.r == 127
        assert purple.g == 0
        assert purple.b == 127
        
        # 0% mix should be original
        same = red.mix(blue, 0.0)
        assert same == red
        
        # 100% mix should be other color
        other = red.mix(blue, 1.0)
        assert other == blue
        
        # Test alpha mixing
        red_alpha = Color(r=255, g=0, b=0, a=0.8)
        blue_alpha = Color(r=0, g=0, b=255, a=0.4)
        mixed = red_alpha.mix(blue_alpha, 0.5)
        assert abs(mixed.a - 0.6) < 1e-10
    
    def test_with_alpha(self):
        """Test alpha modification."""
        color = Color(r=255, g=0, b=0)
        with_alpha = color.with_alpha(0.5)
        
        assert with_alpha.r == color.r
        assert with_alpha.g == color.g
        assert with_alpha.b == color.b
        assert with_alpha.a == 0.5
    
    def test_rotate_hue(self):
        """Test hue rotation."""
        red = Color(r=255, g=0, b=0)
        
        # Rotate 120 degrees should give green
        green = red.rotate_hue(120)
        h, s, l = green.to_hsl()
        assert abs(h - 120) < 1.0
        
        # Rotate 240 degrees should give blue
        blue = red.rotate_hue(240)
        h, s, l = blue.to_hsl()
        assert abs(h - 240) < 1.0
        
        # Rotate 360 degrees should return to original
        back_to_red = red.rotate_hue(360)
        h, s, l = back_to_red.to_hsl()
        assert abs(h - 0) < 1.0
    
    def test_complement(self):
        """Test complementary color."""
        red = Color(r=255, g=0, b=0)
        complement = red.complement()
        
        # Should be cyan (opposite of red)
        h, s, l = complement.to_hsl()
        assert abs(h - 180) < 1.0
    
    def test_is_light_is_dark(self):
        """Test light/dark detection."""
        white = Color(r=255, g=255, b=255)
        black = Color(r=0, g=0, b=0)
        
        assert white.is_light()
        assert not white.is_dark()
        
        assert black.is_dark()
        assert not black.is_light()
        
        # Test with a mid-gray
        gray = Color(r=128, g=128, b=128)
        # This should be determined by luminance calculation
        assert gray.is_light() != gray.is_dark()
        
        # Test with colored examples
        yellow = Color(r=255, g=255, b=0)  # Should be light
        assert yellow.is_light()
        
        navy = Color(r=0, g=0, b=128)  # Should be dark
        assert navy.is_dark()