#!/usr/bin/env python3
"""Download the MOSS audio tokenizer from ModelScope into the local cache."""

from modelscope import snapshot_download


def main() -> None:
    path = snapshot_download("openmoss/MOSS-Audio-Tokenizer")
    print(path)


if __name__ == "__main__":
    main()
