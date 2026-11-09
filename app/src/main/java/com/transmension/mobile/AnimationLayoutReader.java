package com.transmension.mobile;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.zip.DataFormatException;
import java.util.zip.Inflater;

/** Reads only names and frame ranges from a packaged TV reanim, never images. */
final class AnimationLayoutReader {
    private static final int MAX_FILE = 64 * 1024 * 1024;
    private static final int MAX_LAYOUT = 2 * 1024 * 1024;

    static byte[] read(InputStream input) throws IOException {
        ByteArrayOutputStream source = new ByteArrayOutputStream();
        byte[] chunk = new byte[8192];
        for (int size; (size = input.read(chunk)) != -1;) {
            if (source.size() > MAX_FILE - size) throw new IOException("Animation file too large");
            source.write(chunk, 0, size);
        }
        try {
            return decode(source.toByteArray());
        } catch (RuntimeException error) {
            throw new IOException("Invalid animation definition", error);
        }
    }

    private static byte[] unpack(byte[] source) throws IOException {
        if (source.length < 8) throw new IOException("Truncated animation header");
        ByteBuffer header = ByteBuffer.wrap(source).order(ByteOrder.LITTLE_ENDIAN);
        int magic = header.getInt();
        if (magic == 0 && source.length >= 24) return source; // Native also accepts a raw cache.
        if (magic != 0xDEADFED4) throw new IOException("Unknown animation format");
        int length = header.getInt();
        if (length < 24 || length > MAX_FILE) throw new IOException("Invalid animation length");
        byte[] payload = new byte[length];
        Inflater inflater = new Inflater();
        try {
            inflater.setInput(source, 8, source.length - 8);
            int offset = 0;
            while (offset < length && !inflater.finished()) {
                int count = inflater.inflate(payload, offset, length - offset);
                if (count == 0) throw new IOException("Truncated animation payload");
                offset += count;
            }
            if (offset != length || !inflater.finished() || inflater.getRemaining() != 0)
                throw new IOException("Animation payload length mismatch");
        } catch (DataFormatException error) {
            throw new IOException("Invalid compressed animation", error);
        } finally {
            inflater.end();
        }
        return payload;
    }

    static byte[] decode(byte[] source) throws IOException {
        ByteBuffer data = ByteBuffer.wrap(unpack(source)).order(ByteOrder.LITTLE_ENDIAN);
        if (data.getInt() != 0) throw new IOException("Invalid animation schema");
        data.getInt(); // Serialized track pointer; native loader replaces it.
        int tracks = data.getInt();
        data.getFloat(); // FPS does not affect serialized frame ranges.
        data.getInt(); // Serialized atlas pointer.
        if (data.getInt() != 20 || tracks < 0 || tracks > 4096 || tracks > data.remaining() / 20)
            throw new IOException("Invalid animation tracks");
        int[] frames = new int[tracks];
        for (int i = 0; i < tracks; ++i) {
            data.getInt(); // Interned name ID.
            data.getInt(); // Name pointer.
            data.getInt(); // Transform pointer.
            frames[i] = data.getInt();
            data.getInt(); // Attacher flag and padding.
            if (frames[i] < 1 || frames[i] > 1000000)
                throw new IOException("Invalid frame count");
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        putInt(out, tracks == 0 ? 0 : frames[0]);
        putInt(out, tracks);
        for (int i = 0; i < tracks; ++i) {
            byte[] name = string(data, 1024);
            if (data.getInt() != 48 || frames[i] > data.remaining() / 72)
                throw new IOException("Invalid transform block");
            int first = -1, last = -1;
            float visible = 0; // Native FillMissingReanimFrames default.
            for (int frame = 0; frame < frames[i]; ++frame) {
                float value = data.getFloat(data.position() + 24);
                if (Float.isNaN(value) || Float.isInfinite(value)) throw new IOException("Invalid frame value");
                if (value != -10000) visible = value;
                if (visible >= 0) {
                    if (first < 0) first = frame;
                    last = frame;
                }
                data.position(data.position() + 48);
            }
            for (int field = 0; field < frames[i] * 6; ++field) skipString(data);
            putInt(out, name.length);
            out.write(name, 0, name.length);
            putInt(out, frames[i]);
            putInt(out, first < 0 ? 0 : first);
            putInt(out, first < 0 ? 0 : last - first + 1);
            if (out.size() > MAX_LAYOUT) throw new IOException("Animation layout too large");
        }
        if (data.hasRemaining()) throw new IOException("Trailing animation data");
        return out.toByteArray();
    }

    private static byte[] string(ByteBuffer data, int maximum) throws IOException {
        int length = data.getInt();
        if (length < 0 || length > maximum || length > data.remaining()) throw new IOException("Invalid track name");
        byte[] result = new byte[length];
        data.get(result);
        for (byte b : result) if (b == 0) throw new IOException("Null in track name");
        if (!java.util.Arrays.equals(result, new String(result, StandardCharsets.UTF_8).getBytes(StandardCharsets.UTF_8)))
            throw new IOException("Invalid track name encoding");
        return result;
    }
    private static void skipString(ByteBuffer data) throws IOException {
        int length = data.getInt();
        if (length < 0 || length > data.remaining()) throw new IOException("Invalid frame string");
        data.position(data.position() + length);
    }
    private static void putInt(ByteArrayOutputStream out, int value) {
        for (int shift = 0; shift < 32; shift += 8) out.write(value >>> shift);
    }
}
