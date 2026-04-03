//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------
#pragma once

// This is a modification of Godots (non-exposed) RingBuffer class.
// This features a write_override function, so the producer can keep sending data
// and a peek function, so the consumer can see the data without consuming it
// even if the consumer only peeks at it.
// This is useful for the audiocapture of popcorn sound samplers.

#include "godot_cpp/templates/local_vector.hpp"

namespace godot {

template <typename T>
class PopcornFXRingBuffer {
public:
	T read() {
		ERR_FAIL_COND_V(space_left() < 1, T());
		return data.ptr()[inc(read_pos, 1)];
	}

	int peek(T *p_buf, int p_size) {
		int left = data_left();
		p_size = MIN(left, p_size);
		int pos = read_pos;
		int to_read = p_size;
		int dst = 0;
		while (to_read) {
			int end = pos + to_read;
			end = MIN(end, size());
			int total = end - pos;
			const T *read = data.ptr();
			for (int i = 0; i < total; i++) {
				p_buf[dst++] = read[pos + i];
			}
			to_read -= total;
			pos = 0;
		}
		return p_size;
	}

	int read(T *p_buf, int p_size) {
		int left = data_left();
		p_size = MIN(left, p_size);
		int pos = read_pos;
		int to_read = p_size;
		int dst = 0;
		while (to_read) {
			int end = pos + to_read;
			end = MIN(end, size());
			int total = end - pos;
			const T *read = data.ptr();
			for (int i = 0; i < total; i++) {
				p_buf[dst++] = read[pos + i];
			}
			to_read -= total;
			pos = 0;
		}
		inc(read_pos, p_size);
		return p_size;
	}

	int copy(T *p_buf, int p_offset, int p_size) const {
		int left = data_left();
		if ((p_offset + p_size) > left) {
			p_size -= left - p_offset;
			if (p_size <= 0) {
				return 0;
			}
		}
		p_size = MIN(left, p_size);
		int pos = read_pos;
		inc(pos, p_offset);
		int to_read = p_size;
		int dst = 0;
		while (to_read) {
			int end = pos + to_read;
			end = MIN(end, size());
			int total = end - pos;
			for (int i = 0; i < total; i++) {
				p_buf[dst++] = data[pos + i];
			}
			to_read -= total;
			pos = 0;
		}
		return p_size;
	}

	int find(const T &p_t, int p_offset, int p_max_size) const {
		int left = data_left();
		if ((p_offset + p_max_size) > left) {
			p_max_size -= left - p_offset;
			if (p_max_size <= 0) {
				return 0;
			}
		}
		p_max_size = MIN(left, p_max_size);
		int pos = read_pos;
		inc(pos, p_offset);
		int to_read = p_max_size;
		while (to_read) {
			int end = pos + to_read;
			end = MIN(end, size());
			int total = end - pos;
			for (int i = 0; i < total; i++) {
				if (data[pos + i] == p_t) {
					return i + (p_max_size - to_read);
				}
			}
			to_read -= total;
			pos = 0;
		}
		return -1;
	}

	inline int advance_read(int p_n) {
		p_n = MIN(p_n, data_left());
		inc(read_pos, p_n);
		return p_n;
	}

	inline int decrease_write(int p_n) {
		p_n = MIN(p_n, data_left());
		inc(write_pos, size_mask + 1 - p_n);
		return p_n;
	}

	Error write(const T &p_v) {
		ERR_FAIL_COND_V(space_left() < 1, FAILED);
		data[inc(write_pos, 1)] = p_v;
		return OK;
	}

	int write(const T *p_buf, int p_size) {
		int left = space_left();
		p_size = MIN(left, p_size);

		int pos = write_pos;
		int to_write = p_size;
		int src = 0;
		while (to_write) {
			int end = pos + to_write;
			end = MIN(end, size());
			int total = end - pos;

			for (int i = 0; i < total; i++) {
				data[pos + i] = p_buf[src++];
			}
			to_write -= total;
			pos = 0;
		}

		inc(write_pos, p_size);
		return p_size;
	}

	int write_override(const T *p_buf, int p_size) {
		int diff = space_left();
		int pos = write_pos;
		int to_write = p_size;
		int src = 0;
		while (to_write) {
			int end = pos + to_write;
			end = MIN(end, size());
			int total = end - pos;

			for (int i = 0; i < total; i++) {
				data[pos + i] = p_buf[src++];
			}
			to_write -= total;
			pos = 0;
		}

		inc(write_pos, p_size);
		inc(read_pos, MAX(0, p_size - diff));
		return p_size;
	}

	inline int space_left() const {
		int left = read_pos - write_pos;
		if (left < 0) {
			return size() + left - 1;
		}
		if (left == 0) {
			return size() - 1;
		}
		return left - 1;
	}
	inline int data_left() const {
		return size() - space_left() - 1;
	}

	inline int size() const {
		return data.size();
	}

	inline void clear() {
		read_pos = 0;
		write_pos = 0;
	}

	void resize(int p_power) {
		int old_size = size();
		int new_size = 1 << p_power;
		int mask = new_size - 1;
		data.resize(int64_t(1) << int64_t(p_power));
		if (old_size < new_size && read_pos > write_pos) {
			for (int i = 0; i < write_pos; i++) {
				data[(old_size + i) & mask] = data[i];
			}
			write_pos = (old_size + write_pos) & mask;
		} else {
			read_pos = read_pos & mask;
			write_pos = write_pos & mask;
		}

		size_mask = mask;
	}

	PopcornFXRingBuffer(int p_power = 0) {
		resize(p_power);
	}
	~PopcornFXRingBuffer() {}

private:
	TightLocalVector<T> data;
	int read_pos = 0;
	int write_pos = 0;
	int size_mask;

	inline int inc(int &p_var, int p_size) const {
		int ret = p_var;
		p_var += p_size;
		p_var = p_var & size_mask;
		return ret;
	}
};

} //namespace godot
