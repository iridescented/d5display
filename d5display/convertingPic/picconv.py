from PIL import Image
import numpy as np

path = 'UIDESIGND5.jpg'
image = Image.open(path).convert('RGB')
image = np.asarray(image)
print("\n")
print(image)

a_file = open("test.txt", "w")
for row in image:
    #define RGB_CONV(r, g, b) (uint16_t)(((r & 0xf8) << 8) + ((g & 0xfc) << 3) + (b >> 3))

    np.savetxt(a_file, row)

a_file.close()