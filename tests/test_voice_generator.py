import tempfile
import unittest
from pathlib import Path

from tools.generate_retro_robot_voice import apply_retro_effect, read_samples, write_samples


class VoiceGeneratorTest(unittest.TestCase):
    def test_tape_effect_adds_low_noise_bed_to_silent_input(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            source = tmp_path / "source.wav"
            output = tmp_path / "output.wav"
            write_samples(source, 16000, [0] * 1600)

            apply_retro_effect(source, output)

            frame_rate, samples = read_samples(output)
            self.assertEqual(frame_rate, 16000)
            self.assertGreater(len(samples), 1600)
            self.assertGreater(max(abs(sample) for sample in samples), 0)
            self.assertLess(max(abs(sample) for sample in samples), 1000)

    def test_tape_effect_adds_short_hiss_leader_and_tail(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            source = tmp_path / "source.wav"
            output = tmp_path / "output.wav"
            write_samples(source, 16000, [1200] * 1600)

            apply_retro_effect(source, output)

            _, samples = read_samples(output)
            self.assertGreater(len(samples), 1600)


if __name__ == "__main__":
    unittest.main()
