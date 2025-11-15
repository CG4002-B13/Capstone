# Hardware AI

## Hardware

This folder contains all the files for the C++ implementation of the AI model. The function being accelerated in RTL synthesis is ``cnn_accel`` in the ``CNN.cpp`` file.

Under the Ultra96 subfolder, the files that were used on the Ultra96 for deployment can be found. Of note here is ``main.py`` which is the main script running that receives the voice data from MQTT, performs the AI inference on the input data, and outputs the result. ``cnn_inference.py`` contains the AI-related helper functions to preprocess the audio and also perform the AI inference.

## Software

The important files will be covered here, the rest of the files are just helpers.

``CNN.py`` contains the model architecture used in the project.

``train.py`` is used to perform the pre-training on the CNN model.

``finetune.py`` is used to perform the fine-tuning on the CNN model.

``export_weights.py`` is used to extract the model weights for the C++ implementation to use. The generated weights file is ``CNN_weights.h``.