/*
 *  Copyright © 2017-2023 Wellington Wallace
 *
 *  This file is part of Easy Effects.
 *
 *  Easy Effects is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Easy Effects is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Easy Effects. If not, see <https://www.gnu.org/licenses/>.
 */

#include "pitch_ui.hpp"

namespace ui::pitch_box {

struct Data {
 public:
  ~Data() { util::debug("data struct destroyed"); }

  uint serial = 0;

  std::shared_ptr<Pitch> pitch;

  std::vector<sigc::connection> connections;

  std::vector<gulong> gconnections;
};

struct _PitchBox {
  GtkBox parent_instance;

  GtkScale *input_gain, *output_gain;

  GtkLevelBar *input_level_left, *input_level_right, *output_level_left, *output_level_right;

  GtkLabel *input_level_left_label, *input_level_right_label, *output_level_left_label, *output_level_right_label,
      *plugin_credit;

  GtkSpinButton *cents, *semitones, *octaves, *sequence_length, *seek_window, *overlap_length, *tempo_difference,
      *rate_difference;

  GSettings* settings;

  Data* data;
};

// NOLINTNEXTLINE
G_DEFINE_TYPE(PitchBox, pitch_box, GTK_TYPE_BOX)

void on_reset(PitchBox* self, GtkButton* btn) {
  util::reset_all_keys_except(self->settings);
}

void setup(PitchBox* self, std::shared_ptr<Pitch> pitch, const std::string& schema_path) {
  auto serial = get_new_filter_serial();

  self->data->serial = serial;

  g_object_set_data(G_OBJECT(self), "serial", GUINT_TO_POINTER(serial));

  set_ignore_filter_idle_add(serial, false);

  self->data->pitch = pitch;

  self->settings = g_settings_new_with_path(tags::schema::pitch::id, schema_path.c_str());

  pitch->set_post_messages(true);

  self->data->connections.push_back(pitch->input_level.connect([=](const float left, const float right) {
    util::idle_add([=]() {
      if (get_ignore_filter_idle_add(serial)) {
        return;
      }

      update_level(self->input_level_left, self->input_level_left_label, self->input_level_right,
                   self->input_level_right_label, left, right);
    });
  }));

  self->data->connections.push_back(pitch->output_level.connect([=](const float left, const float right) {
    util::idle_add([=]() {
      if (get_ignore_filter_idle_add(serial)) {
        return;
      }

      update_level(self->output_level_left, self->output_level_left_label, self->output_level_right,
                   self->output_level_right_label, left, right);
    });
  }));

  gtk_label_set_text(self->plugin_credit, ui::get_plugin_credit_translated(self->data->pitch->package).c_str());

  gsettings_bind_widgets<"input-gain", "output-gain">(self->settings, self->input_gain, self->output_gain);

  // gsettings_bind_widgets<"mode", "formant", "transients", "detector", "phase">(
  //     self->settings, self->mode, self->formant, self->transients, self->detector, self->phase);

  g_settings_bind(self->settings, "cents", gtk_spin_button_get_adjustment(self->cents), "value",
                  G_SETTINGS_BIND_DEFAULT);

  g_settings_bind(self->settings, "semitones", gtk_spin_button_get_adjustment(self->semitones), "value",
                  G_SETTINGS_BIND_DEFAULT);

  g_settings_bind(self->settings, "octaves", gtk_spin_button_get_adjustment(self->octaves), "value",
                  G_SETTINGS_BIND_DEFAULT);
}

void dispose(GObject* object) {
  auto* self = EE_PITCH_BOX(object);

  self->data->pitch->set_post_messages(false);

  set_ignore_filter_idle_add(self->data->serial, true);

  for (auto& c : self->data->connections) {
    c.disconnect();
  }

  for (auto& handler_id : self->data->gconnections) {
    g_signal_handler_disconnect(self->settings, handler_id);
  }

  self->data->connections.clear();
  self->data->gconnections.clear();

  g_object_unref(self->settings);

  util::debug("disposed");

  G_OBJECT_CLASS(pitch_box_parent_class)->dispose(object);
}

void finalize(GObject* object) {
  auto* self = EE_PITCH_BOX(object);

  delete self->data;

  util::debug("finalized");

  G_OBJECT_CLASS(pitch_box_parent_class)->finalize(object);
}

void pitch_box_class_init(PitchBoxClass* klass) {
  auto* object_class = G_OBJECT_CLASS(klass);
  auto* widget_class = GTK_WIDGET_CLASS(klass);

  object_class->dispose = dispose;
  object_class->finalize = finalize;

  gtk_widget_class_set_template_from_resource(widget_class, tags::resources::pitch_ui);

  gtk_widget_class_bind_template_child(widget_class, PitchBox, input_gain);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, output_gain);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, input_level_left);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, input_level_right);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, output_level_left);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, output_level_right);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, input_level_left_label);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, input_level_right_label);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, output_level_left_label);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, output_level_right_label);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, plugin_credit);

  gtk_widget_class_bind_template_child(widget_class, PitchBox, sequence_length);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, seek_window);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, overlap_length);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, tempo_difference);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, rate_difference);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, cents);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, semitones);
  gtk_widget_class_bind_template_child(widget_class, PitchBox, octaves);

  gtk_widget_class_bind_template_callback(widget_class, on_reset);
}

void pitch_box_init(PitchBox* self) {
  gtk_widget_init_template(GTK_WIDGET(self));

  self->data = new Data();

  prepare_scales<"dB">(self->input_gain, self->output_gain);

  prepare_spinbuttons<"">(self->cents, self->semitones, self->octaves);

  prepare_spinbuttons<"ms">(self->sequence_length, self->seek_window, self->overlap_length);

  prepare_spinbuttons<"%">(self->tempo_difference, self->rate_difference);
}

auto create() -> PitchBox* {
  return static_cast<PitchBox*>(g_object_new(EE_TYPE_PITCH_BOX, nullptr));
}

}  // namespace ui::pitch_box
