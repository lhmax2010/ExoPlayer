package androidx.media3.exoplayer.cppbridge;

import androidx.annotation.Nullable;
import java.util.IdentityHashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Registry used to round-trip arbitrary Java objects through opaque bridge tokens.
 *
 * <p>This registry deduplicates repeated registrations of the same object identity, but it does
 * not currently implement automatic lifecycle cleanup for new object instances created during
 * ongoing playback callbacks. Callers that register external test objects should unregister their
 * tokens explicitly when the objects are no longer needed.
 */
public final class CppOpaqueObjectRegistry {

  private static final ConcurrentHashMap<String, Object> OBJECTS = new ConcurrentHashMap<>();
  private static final Map<Object, String> TOKENS_BY_OBJECT = new IdentityHashMap<>();
  private static final AtomicInteger NEXT_GENERATED_TOKEN_ID = new AtomicInteger(1);

  private CppOpaqueObjectRegistry() {}

  public static synchronized String register(Object object) {
    if (object == null) {
      throw new IllegalArgumentException("object must be non-null");
    }
    String existingToken = TOKENS_BY_OBJECT.get(object);
    if (existingToken != null && OBJECTS.containsKey(existingToken)) {
      return existingToken;
    }
    String token;
    do {
      token = "generated-opaque-object-token-" + NEXT_GENERATED_TOKEN_ID.getAndIncrement();
    } while (OBJECTS.containsKey(token));
    OBJECTS.put(token, object);
    TOKENS_BY_OBJECT.put(object, token);
    return token;
  }

  public static synchronized void register(String token, Object object) {
    if (token == null || token.isEmpty() || object == null) {
      throw new IllegalArgumentException("token and object must be non-null");
    }
    String existingTokenForObject = TOKENS_BY_OBJECT.get(object);
    if (existingTokenForObject != null && !existingTokenForObject.equals(token)) {
      OBJECTS.remove(existingTokenForObject);
    }
    Object previousObject = OBJECTS.put(token, object);
    if (previousObject != null && previousObject != object) {
      TOKENS_BY_OBJECT.remove(previousObject);
    }
    TOKENS_BY_OBJECT.put(object, token);
  }

  public static synchronized void unregister(String token) {
    if (token == null || token.isEmpty()) {
      return;
    }
    Object removedObject = OBJECTS.remove(token);
    if (removedObject != null && token.equals(TOKENS_BY_OBJECT.get(removedObject))) {
      TOKENS_BY_OBJECT.remove(removedObject);
    }
  }

  @Nullable
  public static Object resolve(@Nullable String token) {
    if (token == null || token.isEmpty()) {
      return null;
    }
    return OBJECTS.get(token);
  }
}
