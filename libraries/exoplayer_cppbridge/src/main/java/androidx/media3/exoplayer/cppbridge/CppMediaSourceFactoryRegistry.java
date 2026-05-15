package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;
import androidx.media3.exoplayer.source.MediaSource;
import java.util.IdentityHashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

/** Registry used to resolve Java MediaSource.Factory instances from stable string tokens. */
public final class CppMediaSourceFactoryRegistry {

  private static final ConcurrentHashMap<String, MediaSource.Factory> FACTORIES =
      new ConcurrentHashMap<>();
  private static final Map<MediaSource.Factory, String> TOKENS_BY_FACTORY =
      new IdentityHashMap<>();
  private static final AtomicInteger NEXT_GENERATED_TOKEN_ID = new AtomicInteger(1);

  private CppMediaSourceFactoryRegistry() {}

  public static synchronized String register(MediaSource.Factory factory) {
    if (factory == null) {
      throw new IllegalArgumentException("factory must be non-null");
    }
    String existingToken = TOKENS_BY_FACTORY.get(factory);
    if (existingToken != null && FACTORIES.containsKey(existingToken)) {
      return existingToken;
    }
    String token;
    do {
      token = "generated-media-source-factory-token-" + NEXT_GENERATED_TOKEN_ID.getAndIncrement();
    } while (FACTORIES.containsKey(token));
    FACTORIES.put(token, factory);
    TOKENS_BY_FACTORY.put(factory, token);
    return token;
  }

  public static synchronized void register(String token, MediaSource.Factory factory) {
    if (token == null || token.isEmpty() || factory == null) {
      throw new IllegalArgumentException("token and factory must be non-null");
    }
    String existingTokenForFactory = TOKENS_BY_FACTORY.get(factory);
    if (existingTokenForFactory != null && !existingTokenForFactory.equals(token)) {
      FACTORIES.remove(existingTokenForFactory);
    }
    MediaSource.Factory previousFactory = FACTORIES.put(token, factory);
    if (previousFactory != null && previousFactory != factory) {
      TOKENS_BY_FACTORY.remove(previousFactory);
    }
    TOKENS_BY_FACTORY.put(factory, token);
  }

  public static synchronized void unregister(String token) {
    if (token == null || token.isEmpty()) {
      return;
    }
    MediaSource.Factory removedFactory = FACTORIES.remove(token);
    if (removedFactory != null && token.equals(TOKENS_BY_FACTORY.get(removedFactory))) {
      TOKENS_BY_FACTORY.remove(removedFactory);
    }
  }

  @Nullable
  public static MediaSource.Factory resolve(@Nullable String token) {
    if (token == null || token.isEmpty()) {
      return null;
    }
    return FACTORIES.get(token);
  }
}
