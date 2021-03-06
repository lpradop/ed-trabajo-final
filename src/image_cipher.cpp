#include "image_cipher.hpp"
// genera una secuencia pseudo aleatoria de enteros, cuyos valores se encuentran
// en [0, 2^24)
std::vector<unsigned int> generate_key_sequence(unsigned int size, double x_0,
                                                double lambda) {
  logistic_chaotic_map map{x_0, lambda};
  std::vector<unsigned int> sequence{};
  sequence.resize(size);
  for (unsigned int i{0}; i < size; ++i) {
    sequence[i] = static_cast<int>(0x1'000'000 * map(i));
  }
  return std::move(sequence);
}

// crea un vector de enteros a partir de los datos cargados por stbi_load, para
// cada tripleta r,g,b se realiza la conversion respectiva
std::vector<unsigned int> create_vector_from_image(image const& img) {
  std::vector<unsigned int> tmp{};
  tmp.resize(img.height * img.width);
  int i{0};
  int j{0};
  while (i < img.height * img.width * 3) {
    tmp[j] = rgb_to_decimal(img.data[i], img.data[i + 1], img.data[i + 2]);
    i += 3;
    ++j;
  }
  return std::move(tmp);
}

unsigned int rgb_to_decimal(u_int8_t r, u_int8_t g, u_int8_t b) {
  return (r << 16) + (g << 8) + b;
}

rgb_color decimal_to_rgb(unsigned int color) {
  return rgb_color{static_cast<uint8_t>((color & 0xff0000) >> 16),
                   static_cast<uint8_t>((color & 0x00ff00) >> 8),
                   static_cast<uint8_t>(color & 0x0000ff)};
}

// wrapper para stbi_load, carga la imagen en la memoria y returna la estructura
// image correspondiente
image load_image(char const* filename) {
  image tmp{};
  tmp.data = stbi_load(filename, &tmp.width, &tmp.height, &tmp.channels, 3);
  if (tmp.data == nullptr) {
    throw std::runtime_error{"unable to load the image"};
  }
  return tmp;
};

// guarda la imagen
void write_image(char const* filename, image const& img) {
  stbi_write_png(filename, img.width, img.height, 3, img.data, img.width * 3);
}

// carga la llave, genera x_0 y lambda y a partir de ello genera el vector llave
std::vector<unsigned int> create_vector_from_key(char const* filename,
                                                 int size) {
  std::ifstream key_file{filename};
  std::string key{};
  std::getline(key_file, key);
  auto [x_0, lambda]{generate_seed_and_lambda(key)};
  return std::move(generate_key_sequence(size, x_0, lambda));
}

// almacena los pixeles de un vector dado en una estructura image dada (para
// cada pixel del vector se realiza la conversion a rgb)
void vector_to_raw_data(std::vector<unsigned int> const& pixel_vector,
                        image const& img) {

  int k{0};
  for (unsigned int i{0}; i < pixel_vector.size(); ++i) {
    auto [r, g, b]{decimal_to_rgb(pixel_vector[i])};
    img.data[k] = r;
    img.data[k + 1] = g;
    img.data[k + 2] = b;
    k += 3;
  }
}

// dado un string genera x_0 y lambda usando los 10 primeros caracteres
std::pair<double, double> generate_seed_and_lambda(std::string key) {
  double lambda{0};
  std::bitset<40> even_bits{};
  std::bitset<40> odd_bits{};
  for (int i{0}; i < 10; ++i) {
    std::bitset<8> bitform_key{static_cast<unsigned long long>(key[i])};
    lambda += static_cast<double>(key[i]);
    for (int k{0}; k < 8; k += 2) {
      even_bits[i * 8 + k] = bitform_key[k];
      even_bits[i * 8 + k + 1] = bitform_key[k + 1];
    }
  }
  lambda = (lambda / 0x8'000) + 3.875;

  double seed{static_cast<double>(even_bits.to_ulong() + odd_bits.to_ulong())};
  seed /= 0x10'000'000'000; // 2^40
  double tmp{};
  seed = std::modf(seed, &tmp);
  if (seed == 0) {
    seed += 0.5;
  }
  return std::make_pair(seed, lambda);
}
