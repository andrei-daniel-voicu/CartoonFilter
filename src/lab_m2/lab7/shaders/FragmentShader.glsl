#version 410

// Input
layout(location = 0) in vec2 texture_coord;

// Uniform properties
uniform sampler2D textureImage;
uniform ivec2 screenSize;
uniform int flipVertical;
uniform int outputMode = 2; // 0: original, 1: grayscale, 2: blur

// Output
layout(location = 0) out vec4 out_color;

// Local variables
vec2 textureCoord = vec2(texture_coord.x, (flipVertical != 0) ? 1 - texture_coord.y : texture_coord.y); // Flip texture


vec4 grayscale()
{
    vec4 color = texture(textureImage, textureCoord);
    float gray = 0.21 * color.r + 0.71 * color.g + 0.07 * color.b; 
    return vec4(gray, gray, gray,  0);
}


vec4 blur(int blurRadius)
{
    vec2 texelSize = 1.0f / screenSize;
    vec4 sum = vec4(0);
    for(int i = -blurRadius; i <= blurRadius; i++)
    {
        for(int j = -blurRadius; j <= blurRadius; j++)
        {
            sum += texture(textureImage, textureCoord + vec2(i, j) * texelSize);
        }
    }
        
    float samples = pow((2 * blurRadius + 1), 2);
    return sum / samples;
}

vec4 mean(int radius)
{
   return blur(radius);
}

float grayscale_pixel(vec4 pixel)
{
    return 0.21 * pixel.r + 0.71 * pixel.g + 0.07 * pixel.b;
}

vec4 gaussian()
{
	vec2 texelSize = 1.0f / screenSize;
	vec4 sum = vec4(0);
    float kernel[25] = float[](1, 4, 7, 4, 1, 4, 16, 26, 16, 4, 7, 26, 41, 26, 7, 4, 16, 26, 16, 4, 1, 4, 7, 4, 1);

    for(int i = -2; i <= 2; i++)
	{
		for(int j = -2; j <= 2; j++)
		{
			sum += texture(textureImage, textureCoord + vec2(i, j) * texelSize) * kernel[5 * (i + 2) + (j + 2)];
		}
	}
    return sum / 273;
}


vec4 median(int radius)
{
	vec2 texelSize = 1.0f / screenSize;
    vec4 sort[100];
    for(int i = -radius; i <= radius; i++)
    {
        for(int j = -radius; j <= radius; j++)
        {
            sort[(2 * radius + 1) * (i + 2) + (j + 2)] = texture(textureImage, textureCoord + vec2(i, j) * texelSize);
        }
    }
    for (int i = 0; i < (2 * radius + 1) * (2 * radius + 1) - 1; i++)
	{
		for (int j = i + 1; j < (2 * radius + 1) * (2 * radius + 1); j++)
		{
			if (grayscale_pixel(sort[i]) > grayscale_pixel(sort[j]))
			{
				vec4 aux = sort[i];
				sort[i] = sort[j];
				sort[j] = aux;
			}
		}
	}

	return sort[((2 * radius + 1) * (2 * radius + 1) - 1) / 2];

}

vec4 roberts()
{
    vec2 texelSize = 1.0f / screenSize;
	float gx[9] = float[](0, 0, 0, 0, 1, 0, 0, 0, -1);
	float gy[9] = float[](0, 0, 0, 0, 0, 1, 0, -1, 0);
	vec4 dx = vec4(0);
	vec4 dy = vec4(0);

	for(int i = -1; i <= 1; i++)
		for(int j = -1; j <= 1; j++) {
            float gray = grayscale_pixel(texture(textureImage, textureCoord + vec2(i, j) * texelSize));
			dx += vec4(gray, gray, gray, 1) * gx[3 * (i + 1) + (j + 1)];
        }

	for(int i = -1; i <= 1; i++)
		for(int j = -1; j <= 1; j++) {
            float gray = grayscale_pixel(texture(textureImage, textureCoord + vec2(i, j) * texelSize));
			dy += vec4(gray, gray, gray, 1) * gy[3 * (i + 1) + (j + 1)];
        }
	return sqrt(dx*dx + dy*dy);
}

vec4 sobel() 
{
    vec2 texelSize = 1.0f / screenSize;
	float gx[9] = float[](1, 0, -1, 2, 0, -2, 1, 0, -1);
	float gy[9] = float[](1, 2, 1, 0, 0, 0, -1, -2, -1);
	vec4 dx = vec4(0);
	vec4 dy = vec4(0);

	for(int i = -1; i <= 1; i++)
		for(int j = -1; j <= 1; j++) {
            float gray = grayscale_pixel(texture(textureImage, textureCoord + vec2(i, j) * texelSize));
			dx += vec4(gray, gray, gray, 1) * gx[3 * (i + 1) + (j + 1)];
        }

	for(int i = -1; i <= 1; i++)
		for(int j = -1; j <= 1; j++) {
            float gray = grayscale_pixel(texture(textureImage, textureCoord + vec2(i, j) * texelSize));
			dy += vec4(gray, gray, gray, 1) * gy[3 * (i + 1) + (j + 1)];
        }
	return sqrt(dx*dx + dy*dy);
}

vec4 prewitt()
{
    vec2 texelSize = 1.0f / screenSize;
	float gx[9] = float[](1, 0, -1, 1, 0, -1, 1, 0, -1);
	float gy[9] = float[](1, 1, 1, 0, 0, 0, -1, -1, -1);
	vec4 dx = vec4(0);
	vec4 dy = vec4(0);

	for(int i = -1; i <= 1; i++)
		for(int j = -1; j <= 1; j++) {
            float gray = grayscale_pixel(texture(textureImage, textureCoord + vec2(i, j) * texelSize));
			dx += vec4(gray, gray, gray, 1) * gx[3 * (i + 1) + (j + 1)];
        }

	for(int i = -1; i <= 1; i++)
		for(int j = -1; j <= 1; j++) {
            float gray = grayscale_pixel(texture(textureImage, textureCoord + vec2(i, j) * texelSize));
			dy += vec4(gray, gray, gray, 1) * gy[3 * (i + 1) + (j + 1)];
        }
	return sqrt(dx*dx + dy*dy);    
}

vec4 laplacian()
{
	vec2 texelSize = 1.0f / screenSize;
	float lpl[9] = float[](-1, -1, -1, -1, 8, -1, -1, -1, -1);
	vec4 d = vec4(0);

	for(int i = -1; i <= 1; i++)
		for(int j = -1; j <= 1; j++)
		{
            float gray = grayscale_pixel(texture(textureImage, textureCoord + vec2(i, j) * texelSize));
			d += vec4(gray, gray, gray, 1) * lpl[3 * (i + 1) + (j + 1)];
		}
	return d;
}

vec4 to_grayscale(vec4 color)
{
    float gray = 0.21 * color.r + 0.71 * color.g + 0.07 * color.b;
    return vec4(gray, gray, gray, 1);
}

void main()
{
    switch (outputMode)
    {
        case 1:
        {
            out_color = grayscale();
            break;
        }

        case 2:
        {
            out_color = blur(3);
            break;
        }
        case 3:
        {
            out_color = mean(3);
            break;
        }
        case 4:
        {
            out_color = gaussian();
            break;
        }
        case 5:
        {
            out_color = median(3);
            break;
        }
        case 6:
        {
            out_color = roberts();
            break;
        }
        case 7:
        {
            out_color = sobel();
            break;
        }
        case 8:
        {
            out_color = prewitt();
            break;
        }
        case 9:
        {
            out_color = laplacian();
            break;
        }
        default:
            out_color = texture(textureImage, textureCoord);
            break;
    }
}
