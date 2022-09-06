import numpy as np
import pandas as pd
from datetime import date
from math import ceil
import random
import os
import pickle
from progress.bar import Bar  # pip install progess
from sklearn.model_selection import train_test_split
from sklearn.compose import make_column_transformer
from sklearn.preprocessing import MinMaxScaler
import tensorflow as tf
import matplotlib.pyplot as plt
from tensorflow import keras
from keras import Sequential
from keras.layers import Dense
from keras import backend as K
import seaborn as sns  # plot after normalization
from data_secir_age_groups import splitdata, split_contact_matrices, flat_input
from different_models import *
from tensorflow import keras


def plotCol(inputs, labels, model=None, plot_col='Infected', max_subplots=3):

    # 36 damping entries  and 48 age dependent compartments
    input_width = int((inputs.shape[1] - 36) / 48)
    label_width = int(labels.shape[1] / 48)

    plt.figure(figsize=(12, 8))
    cols = np.array(['Susceptible', 'Exposed', 'Carrier',
                     'Infected', 'Hospitalized', 'ICU', 'Recovered', 'Dead'])
    plot_col_index = np.where(cols == plot_col)[0][0]
    max_n = min(max_subplots, inputs.shape[0])

    # predictions = model(inputs) # for just one input: input_series = tf.expand_dims(inputs[n], axis=0) -> model(input_series)
    for n in range(max_n):
        plt.subplot(max_n, 1, n+1)
        plt.ylabel(f'{plot_col}')

        input_array = inputs[n].numpy()
        label_array = labels[n].numpy()
        plt.plot(np.arange(0, input_width), input_array[plot_col_index:inputs.shape[1] - 36:48],
                 label='Inputs', marker='.', zorder=-10)
        plt.scatter(np.arange(input_width, input_width+label_width), label_array[plot_col_index:-1:48],
                    edgecolors='k', label='Labels', c='#2ca02c', s=64)

        if model is not None:
            input_series = tf.expand_dims(inputs[n], axis=0)
            pred = model(input_series)
            pred = pred[0].numpy()
            plt.scatter(np.arange(input_width, input_width+label_width),
                        pred[plot_col_index:-1:48],
                        marker='X', edgecolors='k', label='Predictions',
                        c='#ff7f0e', s=64)

    plt.xlabel('days')
    plt.show()
    plt.savefig('evaluation_secir_simple_' + plot_col + '.pdf')


def network_fit(path, model, max_epochs=30, early_stop=500):

    if not os.path.isfile(os.path.join(path, 'data_secir_age_groups.pickle')):
        ValueError("no dataset found in path: " + path)

    file = open(os.path.join(path, 'data_secir_age_groups.pickle'), 'rb')

    data = pickle.load(file)
    data_splitted = splitdata(data["inputs"], data["labels"])

    train_inputs_compartments = flat_input(data_splitted["train_inputs"])
    train_labels = flat_input(data_splitted["train_labels"])
    valid_inputs_compartments = flat_input(data_splitted["valid_inputs"])
    valid_labels = flat_input(data_splitted["valid_labels"])
    test_inputs_compartments = flat_input(data_splitted["test_inputs"])
    test_labels = flat_input(data_splitted["test_labels"])

    contact_matrices = split_contact_matrices(tf.stack(data["contact_matrix"]))
    contact_matrices_train = flat_input(contact_matrices['train'])
    contact_matrices_valid = flat_input(contact_matrices['valid'])
    contact_matrices_test = flat_input(contact_matrices['test'])

    train_inputs = tf.concat(
        [train_inputs_compartments, contact_matrices_train], axis=1, name='concat')
    valid_inputs = tf.concat(
        [valid_inputs_compartments, contact_matrices_valid], axis=1, name='concat')
    test_inputs = tf.concat(
        [test_inputs_compartments, contact_matrices_test], axis=1, name='concat')

    early_stopping = tf.keras.callbacks.EarlyStopping(monitor='val_loss',
                                                      patience=early_stop,
                                                      mode='min')

    # mc = ModelCheckpoint("model_secir_age_group" + '.h5', monitor=['mse'],
    #                      mode='min', save_best_only=True)

    model.compile(loss=tf.keras.losses.MeanSquaredError(),
                  optimizer=tf.keras.optimizers.Nadam(),
                  metrics=['mse', 'mae'])

    history = model.fit(train_inputs, train_labels, epochs=max_epochs,
                        validation_data=(
                            valid_inputs, valid_labels),
                        callbacks=[early_stopping])

    plotCol(test_inputs, test_labels, model=model,
            plot_col='Susceptible', max_subplots=3)
    plot_losses(history)
    return history

# simple benchmarking


def timereps(reps, model, input):
    from time import time
    start = time()
    for i in range(0, reps):
        _ = model(input)
    end = time()
    time_passed = end - start
    print(time_passed)
    return (end - start) / reps

# Plot Performance


def plot_histories(histories):
    model_names = ["LSTM", "Dense"]
    count_names = 0
    interval = np.arange(len(histories))
    for x in interval:
        history = histories[x]
        width = 0.3

        train_mae = history.history['mean_absolute_error'][-1]
        valid_mae = history.history['val_mean_absolute_error'][-1]

        plt.bar(x - 0.17, train_mae, width, label='Train')
        plt.bar(x + 0.17, valid_mae, width, label='Validation')
        name = model_names[x]
        # plt.xticks(ticks=x, labels=[name,name],
        #         rotation=45)
        plt.ylabel(f'MAE')
        _ = plt.legend()
        count_names = count_names + 1

    plt.show()
    plt.savefig('evaluation_single_shot.pdf')


def plot_losses(history):
    plt.plot(history.history['loss'])
    plt.plot(history.history['val_loss'])
    plt.title('model loss')
    plt.ylabel('loss')
    plt.xlabel('epoch')
    plt.legend(['train', 'val'], loc='upper left')
    plt.show()
    plt.savefig('losses plot.pdf')


if __name__ == "__main__":
    # TODO: Save contact matrix depending on the damping.
    # In the actual state it might be enough to save the regular one and the damping

    path = os.path.dirname(os.path.realpath(__file__))
    path_data = os.path.join(os.path.dirname(os.path.realpath(
        os.path.dirname(os.path.realpath(path)))), 'data')

    max_epochs = 500

    network_fit(path_data, mlp_model(), max_epochs=max_epochs)
