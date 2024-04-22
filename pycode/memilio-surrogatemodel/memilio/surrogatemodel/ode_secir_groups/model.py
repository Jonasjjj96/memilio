#############################################################################
# Copyright (C) 2020-2023 German Aerospace Center (DLR-SC)
#
# Authors: Agatha Schmidt, Henrik Zunker
#
# Contact: Martin J. Kuehn <Martin.Kuehn@DLR.de>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#############################################################################
import os
import pickle

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import tensorflow as tf

#from memilio.simulation.secir import InfectionState
#from memilio.surrogatemodel.ode_secir_groups import network_architectures


def plot_compartment_prediction_model(
        inputs, labels, modeltype,  model=None,
        plot_compartment='InfectedSymptoms', max_subplots=8):
    """! Plot prediction of the model and label for one compartment. The average of all age groups is plotted. 

    If model is none, we just plot the inputs and labels for the selected compartment without any predictions.  

    @param inputs test inputs for model prediction. 
    @param labels test labels. 
    @param modeltype type of model. Can be 'classic' or 'timeseries'
    @param model trained model. 
    @param plot_col string name of compartment to be plotted. 
    @param max_subplots Number of the simulation runs to be plotted and compared against. 
    """
    if modeltype == 'classic':
        input_width = int((inputs.shape[1] - 37) / 48)
        label_width = int(labels.shape[1])

    elif modeltype == 'timeseries':
        input_width = int(inputs.shape[1])
        label_width = int(labels.shape[1])

    plt.figure(figsize=(12, 8))
    plot_compartment_index = 0
    for compartment in InfectionState.values():
        if compartment.name == plot_compartment:
            break
        plot_compartment_index += 1
    if plot_compartment_index == len(InfectionState.values()):
        raise ValueError('Compartment name given could not be found.')
    max_n = min(max_subplots, inputs.shape[0])

    for n in range(max_n):
        plt.subplot(max_n, 1, n+1)
        plt.ylabel(plot_compartment)

        input_array = inputs[n].numpy()
        label_array = labels[n].numpy()

        if modeltype == 'classic':
            input_plot = input_array[:(input_width*6*8)]
            input_plot = input_plot.reshape(5, 48)

            mean_per_day_input = []
            for i in input_plot:
                x = i[plot_compartment_index::8]
                mean_per_day_input.append(x.mean())

            plt.plot(
                np.arange(0, input_width),
                mean_per_day_input,
                label='Inputs', marker='.', zorder=-10)

        elif modeltype == 'timeseries':
            mean_per_day_input = []
            for i in input_array:
                x = i[plot_compartment_index: inputs.shape[1] - 37:8]
                mean_per_day_input.append(x.mean())

            plt.plot(
                np.arange(0, input_width),
                mean_per_day_input,
                label='Inputs', marker='.', zorder=-10)

        mean_per_day = []
        for i in label_array:
            x = i[plot_compartment_index::8]
            mean_per_day.append(x.mean())
        plt.scatter(
            np.arange(input_width, input_width + label_width),
            mean_per_day,
            edgecolors='k', label='Labels', c='#2ca02c', s=64)

        if model is not None:
            input_series = tf.expand_dims(inputs[n], axis=0)
            pred = model(input_series)
            pred = pred.numpy()
            pred = pred.reshape((30, 48))

            mean_per_day_pred = []
            for i in pred:
                x = i[plot_compartment_index::8]
                mean_per_day_pred.append(x.mean())

            plt.scatter(np.arange(input_width, input_width+pred.shape[-2]),
                        # pred[0, :, plot_compartment_index],
                        mean_per_day_pred,
                        marker='X', edgecolors='k', label='Predictions',
                        c='#ff7f0e', s=64)

    plt.xlabel('days')
    if os.path.isdir("plots") == False:
        os.mkdir("plots")
    plt.savefig('plots/evaluation_secir_groups_' + plot_compartment + '.png')


def network_fit(
        path, filename, model, model_name, modeltype, df_save,  max_epochs=30, early_stop=100, plot=True):
    """! Training and evaluation of a given model with mean squared error loss and Adam optimizer using the mean absolute error as a metric.

    @param path path of the dataset. 
    @param model Keras sequential model.
    @param modeltype type of model. Can be 'classic' or 'timeseries'. Data preparation is made based on the modeltype.
    @param max_epochs int maximum number of epochs in training. 
    @param early_stop Integer that forces an early stop of training if the given number of epochs does not give a significant reduction of validation loss. 

    """

    if not os.path.isfile(os.path.join(path, filename)):
        ValueError("no dataset found in path: " + path)

    file = open(os.path.join(path, filename), 'rb')

    data = pickle.load(file)
    data_splitted = split_data(data['inputs'], data['labels'])

    if modeltype == 'classic':

        train_inputs_compartments = flat_input(data_splitted["train_inputs"])
        train_labels = (data_splitted["train_labels"])
        valid_inputs_compartments = flat_input(data_splitted["valid_inputs"])
        valid_labels = (data_splitted["valid_labels"])
        test_inputs_compartments = flat_input(data_splitted["test_inputs"])
        test_labels = (data_splitted["test_labels"])

        contact_matrices = split_contact_matrices(
            tf.stack(data["contact_matrix"]))
        contact_matrices_train = flat_input(contact_matrices['train'])
        contact_matrices_valid = flat_input(contact_matrices['valid'])
        contact_matrices_test = flat_input(contact_matrices['test'])

        damping_days = data['damping_day']
        damping_days_splitted = split_damping_days(damping_days)
        damping_days_train = damping_days_splitted['train']
        damping_days_valid = damping_days_splitted['valid']
        damping_days_test = damping_days_splitted['test']

        train_inputs = tf.concat(
            [tf.cast(train_inputs_compartments, tf.float32),
             tf.cast(contact_matrices_train, tf.float32),
             tf.cast(damping_days_train, tf.float32)],
            axis=1, name='concat')
        valid_inputs = tf.concat(
            [tf.cast(valid_inputs_compartments, tf.float32),
             tf.cast(contact_matrices_valid, tf.float32),
             tf.cast(damping_days_valid, tf.float32)],
            axis=1, name='concat')
        test_inputs = tf.concat(
            [tf.cast(test_inputs_compartments, tf.float32),
             tf.cast(contact_matrices_test, tf.float32),
             tf.cast(damping_days_test, tf.float32)],
            axis=1, name='concat')

    elif modeltype == 'timeseries':

        train_inputs_compartments = (data_splitted["train_inputs"])
        train_labels = (data_splitted["train_labels"])
        valid_inputs_compartments = (data_splitted["valid_inputs"])
        valid_labels = (data_splitted["valid_labels"])
        test_inputs_compartments = (data_splitted["test_inputs"])
        test_labels = (data_splitted["test_labels"])

        contact_matrices = split_contact_matrices(
            tf.stack(data["contact_matrix"]))
        contact_matrices_train = flat_input(contact_matrices['train'])
        contact_matrices_valid = flat_input(contact_matrices['valid'])
        contact_matrices_test = flat_input(contact_matrices['test'])

        n = np.array(data['damping_day']).shape[0]
        train_days = data['damping_day'][:int(n*0.7)]
        valid_days = data['damping_day'][int(n*0.7):int(n*0.9)]
        test_days = data['damping_day'][int(n*0.9):]

        # concatenate the compartment data with contact matrices and damping days
        # to receive complete input data
        new_contact_train = []
        for i in contact_matrices_train:
            new_contact_train.extend([i for j in range(5)])

        new_contact_train = tf.reshape(
            tf.stack(new_contact_train),
            [train_inputs_compartments.shape[0],
             5, np.asarray(new_contact_train).shape[1]])

        new_damping_days_train = []
        for i in train_days:
            new_damping_days_train.extend([i for j in range(5)])
        new_damping_days_train = tf.reshape(
            tf.stack(new_damping_days_train),
            [train_inputs_compartments.shape[0],
             5, 1])

        train_inputs = tf.concat(
            (tf.cast(train_inputs_compartments, tf.float16),
             tf.cast(new_contact_train, tf.float16),
             tf.cast(new_damping_days_train, tf.float16)),
            axis=2)

        new_contact_test = []
        for i in contact_matrices_test:
            new_contact_test.extend([i for j in range(5)])

        new_contact_test = tf.reshape(tf.stack(new_contact_test), [
            contact_matrices_test.shape[0], 5, contact_matrices_test.shape[1]])

        new_damping_days_test = []
        for i in test_days:
            new_damping_days_test.extend([i for j in range(5)])
        new_damping_days_test = tf.reshape(
            tf.stack(new_damping_days_test),
            [test_inputs_compartments.shape[0],
             5, 1])

        test_inputs = tf.concat(
            (tf.cast(test_inputs_compartments, tf.float16),
             tf.cast(new_contact_test, tf.float16),
             tf.cast(new_damping_days_test, tf.float16)),
            axis=2)

        new_contact_val = []
        for i in contact_matrices_valid:
            new_contact_val.extend([i for j in range(5)])

        new_contact_val = tf.reshape(
            tf.stack(new_contact_val),
            [contact_matrices_valid.shape[0],
             5, contact_matrices_valid.shape[1]])

        new_damping_days_valid = []
        for i in valid_days:
            new_damping_days_valid.extend([i for j in range(5)])
        new_damping_days_valid = tf.reshape(
            tf.stack(new_damping_days_valid),
            [valid_inputs_compartments.shape[0],
             5, 1])

        valid_inputs = tf.concat(
            (tf.cast(valid_inputs_compartments, tf.float16),
             tf.cast(new_contact_val, tf.float16),
             tf.cast(new_damping_days_valid, tf.float16)),
            axis=2)

    batch_size = 32

    early_stopping = tf.keras.callbacks.EarlyStopping(monitor='val_loss',
                                                      patience=early_stop,
                                                      mode='min')

    model.compile(
        loss=tf.keras.losses.MeanAbsolutePercentageError(),
        optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
        metrics=[tf.keras.metrics.MeanSquaredError()])

    history = model.fit(train_inputs, train_labels, epochs=max_epochs,
                        validation_data=(valid_inputs, valid_labels),
                        batch_size=batch_size,
                        callbacks=[early_stopping])
    

    # save the model
    path = os.path.dirname(os.path.realpath(__file__))
    path_models = os.path.join(
        os.path.dirname(
            os.path.realpath(os.path.dirname(os.path.realpath(path)))),
        'saved_models_groups_100days_1damp_CNN_w')
    if not os.path.isdir(path_models):
        os.mkdir(path_models)

    model.save(path_models, 'CNN_100days_groups_onedamp_w.h5')

    if (plot):
        #plot_losses(history)
        #plot_compartment_prediction_model(
        #    test_inputs, test_labels, modeltype, model=model,
        #    plot_compartment='InfectedSymptoms', max_subplots=3)
        df = get_test_statistic(test_inputs, test_labels, model)
        print(df)
        print('mean: ',  df.mean())
        
        
        filename_df = 'datarame_secirgroups_10days__1damp_w_CNN'
        df_save.loc[len(df_save.index)] = [df.mean()[0], model_name]
    
        path = os.path.dirname(os.path.realpath(__file__))
        file_path = os.path.join(
            os.path.dirname(
                os.path.realpath(os.path.dirname(os.path.realpath(path)))),
            'secir_groups_W')
        if not os.path.isdir(file_path):
            os.mkdir(file_path)
        file_path = os.path.join(file_path,filename_df)
        df_save.to_csv(file_path)


    return history


def plot_losses(history):
    """! Plots the losses of the model training.  

    @param history model training history. 

    """
    plt.plot(history.history['loss'])
    plt.plot(history.history['val_loss'])
    plt.title('model loss')
    plt.ylabel('loss')
    plt.xlabel('epoch')
    plt.legend(['train', 'val'], loc='upper left')
    if os.path.isdir("plots") == False:
        os.mkdir("plots")
    plt.savefig('plots/losses_plot.png')
    plt.show()


def get_test_statistic(test_inputs, test_labels, model):
    """! Calculates the mean absolute percentage error based on the test dataset.   

    @param test_inputs inputs from test data.
    @param test_labels labels (output) from test data.
    @param model trained model. 

    """

    pred = model(test_inputs)
    pred = pred.numpy()
    test_labels = np.array(test_labels)

    diff = pred - test_labels
    relative_err = (abs(diff))/abs(test_labels)
    # reshape [batch, time, features] -> [features, time * batch]
    relative_err_transformed = relative_err.transpose(2, 0, 1).reshape(8, -1)
    relative_err_means_percentage = relative_err_transformed.mean(axis=1) * 100

    # delete the two confirmed compartments from InfectionStates
    #compartment_array = []
    #for compartment in InfectionState.values():
    #    compartment_array.append(compartment) 
    #index = [3,5]
    #compartments_cleaned= np.delete(compartment_array, index)

    #mean_percentage = pd.DataFrame(
    #    data=relative_err_means_percentage,
    #    index=[str(compartment).split('.')[1]
    #           for compartment in compartments_cleaned],
    #    columns=['Percentage Error'])




    infectionstates = ['Susceptible','Exposed', 'InfectedNoSymptoms', 'InfectedSymptoms', 'InfectedSevere', 'InfectedCritical', 'Receovered', 'Dead']
    compartment_array = []
    #for compartment in InfectionState.values():
    #    compartment_array.append(compartment) 
    #index = [3,5]
    #compartments_cleaned= np.delete(compartment_array, index)
    for compartment in infectionstates:
        compartment_array.append(compartment) 

    mean_percentage = pd.DataFrame(
        data=relative_err_means_percentage,
        #index=[str(compartment).split('.')[1]
        #       for compartment in compartments_cleaned],
        index = infectionstates, 
        columns=['Percentage Error'])


    return mean_percentage


def split_data(inputs, labels, split_train=0.7,
               split_valid=0.2, split_test=0.1):
    """! Split data set in training, validation and testing data sets.

   @param inputs input dataset
   @param labels label dataset
   @param split_train Share of training data sets.
   @param split_valid Share of validation data sets.
   @param split_test Share of testing data sets.
   """

    if split_train + split_valid + split_test > 1 + 1e-10:
        raise ValueError(
            "Summed data set shares are greater than 1. Please adjust the values.")
    elif inputs.shape[0] != labels.shape[0] or inputs.shape[2] != labels.shape[2]:
        raise ValueError(
            "Number of batches or features different for input and labels")

    n = inputs.shape[0]
    n_train = int(n * split_train)
    n_valid = int(n * split_valid)
    n_test = n - n_train - n_valid

    inputs_train, inputs_valid, inputs_test = tf.split(
        inputs, [n_train, n_valid, n_test], 0)
    labels_train, labels_valid, labels_test = tf.split(
        labels, [n_train, n_valid, n_test], 0)

    data = {
        'train_inputs': inputs_train,
        'train_labels': labels_train,
        'valid_inputs': inputs_valid,
        'valid_labels': labels_valid,
        'test_inputs': inputs_test,
        'test_labels': labels_test
    }

    return data







def flat_input(input):
    """! Flatten input dimension

   @param input input array

   """
    dim = tf.reduce_prod(tf.shape(input)[1:])
    return tf.reshape(input, [-1, dim])


def split_contact_matrices(contact_matrices, split_train=0.7,
                           split_valid=0.2, split_test=0.1):
    """! Split dampings in train, valid and test

   @param contact_matrices contact matrices
   @param labels label dataset
   @param split_train ratio of train datasets
   @param split_valid ratio of validation datasets
   @param split_test ratio of test datasets
   """

    if split_train + split_valid + split_test != 1:
        ValueError("summed Split ratios not equal 1! Please adjust the values")

    n = contact_matrices.shape[0]
    n_train = int(n * split_train)
    n_valid = int(n * split_valid)
    n_test = int(n * split_test)

    if n_train + n_valid + n_test != n:
        n_test = n - n_train - n_valid

    contact_matrices_train, contact_matrices_valid, contact_matrices_test = tf.split(
        contact_matrices, [n_train, n_valid, n_test], 0)
    data = {
        "train": contact_matrices_train,
        "valid": contact_matrices_valid,
        "test": contact_matrices_test
    }

    return data


def split_damping_days(damping_days, split_train=0.7,
                       split_valid=0.2, split_test=0.1):
    """! Split damping days in train, valid and test

   @param damping_days damping days
   @param split_train ratio of train datasets
   @param split_valid ratio of validation datasets
   @param split_test ratio of test datasets
   """

    if split_train + split_valid + split_test != 1:
        ValueError("summed Split ratios not equal 1! Please adjust the values")
    damping_days = np.asarray(damping_days)
    n = damping_days.shape[0]
    n_train = int(n * split_train)
    n_valid = int(n * split_valid)
    n_test = int(n * split_test)

    if n_train + n_valid + n_test != n:
        n_test = n - n_train - n_valid

    damping_days_train, damping_days_valid, damping_days_test = tf.split(
        damping_days, [n_train, n_valid, n_test], 0)
    data = {
        "train": tf.reshape(damping_days_train, [n_train, 1]),
        "valid": tf.reshape(damping_days_valid, [n_valid, 1]),
        "test": tf.reshape(damping_days_test, [n_test, 1])
    }

    return data


#def get_input_dim_lstm(path, filename):
#    """! Extract the dimensiond of the input data#

   #@param path path to the data 

   #"""
   # file = open(os.path.join(path, filename), 'rb')

    #data = pickle.load(file)
    #input_dim = data['inputs'].shape[2] + np.asarray(
    #    data['contact_matrix']).shape[1] * np.asarray(data['contact_matrix']).shape[2]+1

    #return input_dim

def get_input_dim_lstm(path, filename):
    """! Extract the dimensiond of the input data

   @param path path to the data 

   """
    file = open(os.path.join(path, filename), 'rb')

    data = pickle.load(file)
    input_dim = data['inputs'].shape[2] 

    return input_dim




def get_output_dim_lstm(path, filename):
    """! Extract the dimensiond of the input data

   @param path path to the data 

   """
    file = open(os.path.join(path, filename), 'rb')

    data = pickle.load(file)
    output_dim = data['labels'].shape[2] * data['labels'].shape[1]

    return output_dim


def get_dim_classic(path, name):
    """! Extract the  input and output dimensions of the data for the classic models

   @param path path to the data 
   @param file file name

   """
    file = open(os.path.join(path, name ), 'rb')

    data = pickle.load(file)
    #input_dim = data['inputs'].shape[1]*data['inputs'].shape[2]+ np.asarray(data['contact_matrix']).shape[1]* np.asarray(data['contact_matrix']).shape[2]* np.asarray(data['contact_matrix']).shape[3]+np.asarray(data['damping_day']).shape[1]
    input_dim = 277
    #output_dim = data['labels'].shape[1]*data['labels'].shape[2]
    output_dim = 4800
    
    return input_dim, output_dim


if __name__ == "__main__":
    path = os.path.dirname(os.path.realpath(__file__))
    path_data = os.path.join(os.path.dirname(os.path.realpath(
        os.path.dirname(os.path.realpath(path)))), 'data')
    
    filename = 'data_secir_groups_100days_onedamp_w.pickle'
    df_save = pd.DataFrame(columns = ['MAPE', 'Model'])

    input_dim_classic, output_dim_classic = get_dim_classic(path_data,filename)

    input_dim = get_input_dim_lstm(path_data, filename)
    output_dim = get_output_dim_lstm(path_data, filename)   



    max_epochs = 1500
    label_width = 100

    #input_dim = get_input_dim_lstm(path_data, filename)

    def lstm_multi_input_multi_output(label_width, num_age_groups=6):
        """! LSTM Network which uses multiple time steps as input and returns the 8 compartments for
        one single time step in the future.

        Input and output have shape [number of expert model simulations, time points in simulation,
        number of individuals in infection states].

        @param label_width Number of time steps in the output.
        """
        model = tf.keras.Sequential([
            tf.keras.layers.LSTM(512, return_sequences=False),
            tf.keras.layers.Dense(label_width * 8 * num_age_groups,
                                kernel_initializer=tf.initializers.zeros()),
            tf.keras.layers.Reshape([label_width, 8 * num_age_groups])])
        return model
    
    def mlp_multi_input_multi_output(label_width, num_age_groups=6):
        """! Simple MLP Network which takes the compartments for multiple time steps as input and
        returns the 8 compartments for all age groups for multiple time steps in the future. 

        Reshaping adds an extra dimension to the output, so the shape of the output is 30x48.
        This makes the shape comparable to that of the multi-output models.

        @param label_width Number of time steps in the output.
        @param num_age_groups Number of age groups in population.
        """
        model = tf.keras.Sequential([
            tf.keras.layers.Flatten(),
            tf.keras.layers.Dense(units=1024, activation='relu'),
            tf.keras.layers.Dense(units=1024, activation='relu'),
            tf.keras.layers.Dense(units=1024, activation='relu'),
            tf.keras.layers.Dense(units=1024, activation='relu'),
            tf.keras.layers.Dense(units=label_width*num_age_groups*8),
            tf.keras.layers.Reshape([label_width, num_age_groups*8])
        ])
        return model
    


    def cnn_model(input_dim, output_dim):

                    model = tf.keras.Sequential()
                    model.add(
                        tf.keras.layers.Conv1D(
                            filters=64, kernel_size=3, activation='relu',
                            input_shape=(input_dim, 1)))  # 312
                    model.add(tf.keras.layers.Conv1D(filters=64, kernel_size=3, activation='relu'))
                    model.add(tf.keras.layers.Dropout(0.5))
                    model.add(tf.keras.layers.MaxPooling1D(pool_size=2))
                    model.add(tf.keras.layers.Flatten())
                    #model.add(GaussianNoise(0.35)) 
                    model.add(tf.keras.layers.BatchNormalization())
                    model.add(tf.keras.layers.Dense(1024, activation='relu'))
                    model.add(tf.keras.layers.BatchNormalization())
                    model.add(tf.keras.layers.Dense(1024, activation='relu'))            

                    model.add(tf.keras.layers.Dense(output_dim, activation='linear'))  # 1440
                    model.add(tf.keras.layers.Reshape([100, 6*8]))
                    return model

        
    model = cnn_model(input_dim_classic,output_dim_classic)

    #models = ['Dense','LSTM', 'CNN']

    model_name = "CNN"
    modeltype = 'classic'
    #model = cnn_multi_input_multi_output(label_width)
    #for model_name in models:
    #    model = model_name
    #    if model == "Dense_Single":
    #        model = network_architectures.mlp_multi_input_single_output()
    #        modeltype = 'classic'

        #elif model == "Dense":
        #    model = network_architectures.mlp_multi_input_multi_output(label_width)
        #    modeltype = 'classic'

        #elif model == "LSTM":
        #    model = network_architectures.lstm_multi_input_multi_output(
        #    label_width)
        #    modeltype = 'timeseries'

        #elif model == "CNN":
        #    model = network_architectures.cnn_multi_input_multi_output(label_width)
        #    modeltype = 'timeseries'

        #for i in range(5):

    model_output = network_fit(
            path_data, filename, model=model,model_name = model_name,  modeltype=modeltype, df_save = df_save,
            max_epochs=max_epochs)
