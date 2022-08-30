import numpy as np
import pandas as pd
from datetime import date
from math import ceil
import random
import os
import pickle
from progress.bar import Bar # pip install progess
from sklearn.model_selection import train_test_split
from sklearn.compose import make_column_transformer
from sklearn.preprocessing import MinMaxScaler
import tensorflow as tf
import matplotlib.pyplot as plt
from tensorflow import keras
from keras import Sequential
from keras.layers import Dense
from keras.optimizers import Adam
from keras import backend as K
import seaborn as sns # plot after normalization
from data_secir_simple import generate_data, splitdata
from different_models import *
from sklearn.metrics import mean_absolute_percentage_error


def plotCol(inputs, labels, model=None, plot_col='Infected', max_subplots=8):

    input_width = inputs.shape[1]
    label_width = labels.shape[1]
    
    plt.figure(figsize=(12, 8))
    cols = np.array(['Exposed', 'Carrier',
                        'Infected', 'Hospitalized', 'ICU', 'Dead'])  
    plot_col_index = np.where(cols == plot_col)[0][0]
    max_n = min(max_subplots, inputs.shape[0])

    # predictions = model(inputs) # for just one input: input_series = tf.expand_dims(inputs[n], axis=0) -> model(input_series)
    for n in range(max_n):
        plt.subplot(max_n, 1, n+1)
        plt.ylabel(f'{plot_col}')

        input_array = inputs[n].numpy()
        label_array = labels[n].numpy()
        plt.plot(np.arange(0,input_width), input_array[:, plot_col_index],
                label='Inputs', marker='.', zorder=-10)
        plt.scatter(np.arange(input_width,input_width+label_width), label_array[:,plot_col_index],
                    edgecolors='k', label='Labels', c='#2ca02c', s=64)
        
        if model is not None:
            input_series = tf.expand_dims(inputs[n], axis=0)
            pred = model(input_series)
            pred = pred.numpy()
            plt.scatter(np.arange(input_width,input_width+pred.shape[-2]), 
                                pred[0,:,plot_col_index],
                                marker='X', edgecolors='k', label='Predictions',
                                c='#ff7f0e', s=64)

    plt.xlabel('days')
    plt.show()
    plt.savefig('evaluation_secir_simple_' + plot_col + '.pdf')

def network_fit(path, model,  max_epochs=30, early_stop=500, save_evaluation_pdf=False):

    if not os.path.isfile(os.path.join(path, 'data_secir_simple.pickle')):
        ValueError("no dataset found in path: " + path)

   
    file = open(os.path.join(path, 'data_secir_simple.pickle'),'rb')


    data = pickle.load(file)



    data_splitted = splitdata(data["inputs"], data["labels"])

    train_inputs = data_splitted["train_inputs"]
    train_labels = data_splitted["train_labels"]
    valid_inputs = data_splitted["valid_inputs"]
    valid_labels = data_splitted["valid_labels"]
    test_inputs  = data_splitted["test_inputs"]
    test_labels  = data_splitted["test_labels"]



    early_stopping = tf.keras.callbacks.EarlyStopping(monitor='val_loss',
                                                    patience=early_stop,
                                                    mode='min')

    
    model.compile(#loss=tf.keras.losses.MeanAbsolutePercentageError(),
                loss=tf.keras.losses.MeanSquaredError(),
                optimizer=tf.keras.optimizers.Adam(),
                metrics=[tf.keras.metrics.MeanAbsoluteError()])
                #metrics=[tf.keras.metrics.MeanAbsolutePercentageError()])



    history = model.fit(train_inputs, train_labels, epochs=max_epochs,
                      validation_data=(valid_inputs, valid_labels),
                      callbacks=[early_stopping])
    
    plot_losses(history)
    plotCol(test_inputs, test_labels, model=model, plot_col='Infected', max_subplots=6)
    plotCol(test_inputs, test_labels, model=model, plot_col='Exposed', max_subplots=6)
    plotCol(test_inputs, test_labels, model=model, plot_col='Dead', max_subplots=6)
    #plot_losses(history)
    df = get_test_statistic(test_inputs, test_labels, model)
    print(df)
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


def get_test_statistic(test_inputs, test_labels, model): 
        pred = model(test_inputs)
        pred = pred.numpy()
        test_labels = np.array(test_labels)

        diff = pred - test_labels
        anteil = (abs(diff))/abs(test_labels)

        anteil_6 = anteil.transpose(2,0,1).reshape(6,-1)
        df = pd.DataFrame(data = anteil_6)
        df = df.transpose()

        mean_percentage = pd.DataFrame(data = (df.mean().values)*100 , index = ['Exposed', 'Carrier',
                   'Infected', 'Hospitalized', 'ICU', 'Dead'], columns = ['Percentage Error'])


    

        return mean_percentage


if __name__ == "__main__":
    # TODO: Save contact matrix depending on the damping.
    # In the actual state it might be enough to save the regular one and the damping

    

    path = os.path.dirname(os.path.realpath(__file__))
    path_data = os.path.join(os.path.dirname(os.path.realpath(path)), 'data')

    max_epochs = 500

    ### Models ###
    # single input
    # hist = network_fit(path_data, model=single_output(), max_epochs=max_epochs)

    # # multi input
    # lstm_hist = network_fit(path_data, model=lstm_network_multi_input(), max_epochs=max_epochs)
    # ml_hist = network_fit(path_data, model=multilayer_multi_input(), max_epochs=max_epochs)

    # # Multi output 
    cnn_output = network_fit(path_data, model=cnn_multi_output(), max_epochs=max_epochs)


    #lstm_hist_multi = network_fit(path_data, model=lstm_multi_output(), max_epochs=max_epochs)
    
    # histories = [ lstm_hist, ml_hist]
    # plot_histories(histories)
