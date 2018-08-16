#include "modules/custom.hpp"
#include <iostream>

waybar::modules::Custom::Custom(const std::string &name, Json::Value config)
  : name_(name), config_(std::move(config))
{
  if (!config_["exec"]) {
    throw std::runtime_error(name_ + " has no exec path.");
  }
  if (config_["max-length"]) {
    label_.set_max_width_chars(config_["max-length"].asUInt());
    label_.set_ellipsize(Pango::EllipsizeMode::ELLIPSIZE_END);
  }
  uint32_t interval = config_["interval"] ? config_["inveral"].asUInt() : 30;
  thread_ = [this, interval] {
    Glib::signal_idle().connect_once(sigc::mem_fun(*this, &Custom::update));
    thread_.sleep_for(chrono::seconds(interval));
  };
};

auto waybar::modules::Custom::update() -> void
{
  std::array<char, 128> buffer = {0};
  std::string output;
  std::shared_ptr<FILE> fp(popen(config_["exec"].asCString(), "r"), pclose);
  if (!fp) {
    std::cerr << name_ + " can't exec " + config_["exec"].asString() << std::endl;
    return;
  }

  while (feof(fp.get()) == 0) {
    if (fgets(buffer.data(), 128, fp.get()) != nullptr) {
        output += buffer.data();
    }
  }

  // Remove last newline
  if (!output.empty() && output[output.length()-1] == '\n') {
    output.erase(output.length()-1);
  }

  // Hide label if output is empty
  if (output.empty()) {
    label_.hide();
    label_.set_name("");
  } else {
    label_.set_name("custom-" + name_);
    auto format = config_["format"] ? config_["format"].asString() : "{}";
    auto str = fmt::format(format, output);
    label_.set_text(str);
    label_.set_tooltip_text(str);
    label_.show();
  }
}

waybar::modules::Custom::operator Gtk::Widget &() {
  return label_;
}