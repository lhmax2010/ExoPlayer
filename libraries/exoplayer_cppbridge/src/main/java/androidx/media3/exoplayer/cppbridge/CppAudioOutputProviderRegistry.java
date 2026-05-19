package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;
import androidx.media3.exoplayer.audio.AudioOutputProvider;
import java.util.IdentityHashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

/** Registry that lets native code select app-owned AudioOutputProvider instances by token. */
public final class CppAudioOutputProviderRegistry {

  private static final ConcurrentHashMap<String, AudioOutputProvider> PROVIDERS =
      new ConcurrentHashMap<>();
  private static final Map<AudioOutputProvider, String> TOKENS_BY_PROVIDER =
      new IdentityHashMap<>();
  private static final AtomicInteger NEXT_GENERATED_TOKEN_ID = new AtomicInteger(1);

  private CppAudioOutputProviderRegistry() {}

  public static synchronized String register(AudioOutputProvider audioOutputProvider) {
    if (audioOutputProvider == null) {
      throw new IllegalArgumentException("audioOutputProvider must be non-null");
    }
    String existingToken = TOKENS_BY_PROVIDER.get(audioOutputProvider);
    if (existingToken != null && PROVIDERS.containsKey(existingToken)) {
      return existingToken;
    }
    String token;
    do {
      token = "generated-audio-output-provider-token-" + NEXT_GENERATED_TOKEN_ID.getAndIncrement();
    } while (PROVIDERS.containsKey(token));
    register(token, audioOutputProvider);
    return token;
  }

  public static synchronized void register(String token, AudioOutputProvider audioOutputProvider) {
    if (token == null || token.isEmpty() || audioOutputProvider == null) {
      throw new IllegalArgumentException("token and audioOutputProvider must be non-null");
    }
    String existingTokenForProvider = TOKENS_BY_PROVIDER.get(audioOutputProvider);
    if (existingTokenForProvider != null && !existingTokenForProvider.equals(token)) {
      PROVIDERS.remove(existingTokenForProvider);
    }
    AudioOutputProvider previousProvider = PROVIDERS.put(token, audioOutputProvider);
    if (previousProvider != null && previousProvider != audioOutputProvider) {
      TOKENS_BY_PROVIDER.remove(previousProvider);
    }
    TOKENS_BY_PROVIDER.put(audioOutputProvider, token);
  }

  public static synchronized void unregister(String token) {
    if (token == null || token.isEmpty()) {
      return;
    }
    AudioOutputProvider removedProvider = PROVIDERS.remove(token);
    if (removedProvider != null && token.equals(TOKENS_BY_PROVIDER.get(removedProvider))) {
      TOKENS_BY_PROVIDER.remove(removedProvider);
    }
  }

  public static @Nullable AudioOutputProvider resolve(@Nullable String token) {
    if (token == null || token.isEmpty()) {
      return null;
    }
    return PROVIDERS.get(token);
  }
}
