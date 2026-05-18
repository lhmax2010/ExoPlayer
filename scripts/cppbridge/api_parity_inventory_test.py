import textwrap
import unittest

import api_parity_inventory as inventory


class ApiParityInventoryTest(unittest.TestCase):

    def test_extract_api_methods_handles_nested_signature_class_names(self):
        api_text = textwrap.dedent(
            """
            // Signature format: 2.0
            package androidx.media3.common {

              public interface Player {
                method @IntRange(from=0, to=100) public int getBufferedPercentage();
                method public void play();
                method @Nullable public androidx.media3.common.MediaItem getCurrentMediaItem();
                method public void setVolume(@FloatRange(from=0, to=1.0) float);
              }

              public static interface Player.Listener {
                method public default void onTracksChanged(androidx.media3.common.Tracks);
              }

            }
            """
        )

        methods = inventory.extract_api_methods(
            api_text,
            (
                "androidx.media3.common.Player",
                "androidx.media3.common.Player.Listener",
            ),
        )

        self.assertEqual(
            [method.method_name for method in methods["androidx.media3.common.Player"]],
            ["getBufferedPercentage", "play", "getCurrentMediaItem", "setVolume"],
        )
        self.assertEqual(
            [method.method_name for method in methods["androidx.media3.common.Player.Listener"]],
            ["onTracksChanged"],
        )

    def test_extract_java_bridge_methods_keeps_top_level_methods_only(self):
        java_text = textwrap.dedent(
            """
            public final class CppExoPlayerBridge {
              public void play() {}
              public int getDeviceVolumeValue() { return 0; }
              public void setAudioAttributesConfig(int usage, int contentType) {}
              private void helper() {}
                public void innerCallbackShouldNotCount() {}
            }
            """
        )

        methods = inventory.extract_java_bridge_methods(java_text)

        self.assertIn("play", methods)
        self.assertIn("getDeviceVolume", methods)
        self.assertIn("setAudioAttributes", methods)
        self.assertNotIn("innerCallbackShouldNotCount", methods)

    def test_extract_cpp_methods_handles_multiline_virtual_declarations(self):
        header_text = textwrap.dedent(
            """
            class ExoPlayerSdkPlayer {
             public:
              virtual ~ExoPlayerSdkPlayer() = default;
              virtual void SetMediaItem(
                  const MediaItemDescriptor& media_item,
                  bool reset_position) = 0;
              virtual bool IsPlaying() = 0;
            };
            """
        )

        methods = inventory.extract_cpp_methods(header_text, ("ExoPlayerSdkPlayer",))

        self.assertEqual(methods, {"setMediaItem", "isPlaying"})

    def test_extract_cpp_methods_handles_nonvirtual_builder_methods(self):
        header_text = textwrap.dedent(
            """
            class ExoPlayerSdkPlayerBuilder {
             public:
              ExoPlayerSdkPlayerBuilder() = default;

              ExoPlayerSdkPlayerBuilder& SetSeekBackIncrementMs(int64_t value);
              ExoPlayerSdkPlayerBuilder& SetMediaSourceFactoryConfig(
                  const PlayerConfig::MediaSourceFactoryConfig& config);
              std::unique_ptr<ExoPlayerSdkPlayer> Build(JNIEnv* env, jobject context) const;

             private:
              PlayerConfig config_;
            };
            """
        )

        methods = inventory.extract_cpp_methods(header_text, ("ExoPlayerSdkPlayerBuilder",))

        self.assertEqual(
            methods,
            {"setSeekBackIncrementMs", "setMediaSourceFactoryConfig", "build"},
        )

    def test_extract_cpp_methods_handles_inline_default_listener_methods(self):
        header_text = textwrap.dedent(
            """
            class PlayerListener {
             public:
              virtual ~PlayerListener() = default;
              virtual void OnPlayerErrorChanged(const PlaybackSnapshot& snapshot) {}
              virtual void OnTimelineChanged(
                  const PlaybackSnapshot& snapshot,
                  const TimelineDetailsSnapshot& timeline,
                  int reason) {}
            };
            """
        )

        methods = inventory.extract_cpp_methods(header_text, ("PlayerListener",))

        self.assertEqual(methods, {"onPlayerErrorChanged", "onTimelineChanged"})

    def test_uncovered_methods_respects_known_aliases(self):
        missing = inventory.uncovered_methods(
            {"getCurrentTracks", "setAudioOutputProvider"},
            {"getTracks"},
        )

        self.assertEqual(missing, ["setAudioOutputProvider"])


if __name__ == "__main__":
    unittest.main()
